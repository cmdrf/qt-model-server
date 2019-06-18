#include "JsonViewModel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QAbstractItemModel>
#include <QDebug>
#include <QMetaMethod>

namespace qtmodelserver
{

JsonViewModel::JsonViewModel(QObject* parent) : QObject(parent)
{

}

void JsonViewModel::sendEntireData()
{
	const int rowCount = m_model ? m_model->rowCount() : 0;

	QJsonObject outObject;
	if(useRowBasedProtocol())
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("rowData"));
		outObject.insert(QStringLiteral("items"), fetchRowsAsArray(0, rowCount - 1));
		outObject.insert(QStringLiteral("key"), keyName());
	}
	else
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("data"));
		outObject.insert(QStringLiteral("items"), fetchRows(0, rowCount - 1));
	}
	QJsonDocument outDocument(outObject);
	sendMessage(outDocument);
}

void JsonViewModel::receiveMessage(const QString& message)
{
	receiveMessage(message.toUtf8());
}

void JsonViewModel::receiveMessage(const QByteArray& message)
{
	if(!m_model)
		return;

	auto document = QJsonDocument::fromJson(message);
	if(!document.isObject())
	{
		qWarning() << "Message is not a JSON object";
		return;
	}
	QJsonObject object = document.object();
	auto operationIt = object.find("operation");
	if(operationIt == object.end())
	{
		qWarning() << "No operation in message";
		return;
	}

	if(!operationIt->isString())
	{
		qWarning() << "Operation is not a string";
		return;
	}

	auto itemsIt = object.find("items");
	if(itemsIt == object.end())
	{
		qWarning() << "No items in message";
		return;
	}

	auto operationString = operationIt->toString();
	if(operationString == "changeData")
	{
		if(!itemsIt->isObject())
		{
			qWarning() << "items is not an object";
			return;
		}
		QJsonObject items = itemsIt->toObject();
		for(auto itemIt = items.begin(); itemIt != items.end(); ++itemIt)
		{
			int row = getRowForKey(itemIt.key());
			if(row < 0)
			{
				qDebug() << "Row for" << itemIt.key() << "not found";
				continue;
			}
			if(!itemIt->isObject())
			{
				qWarning() << "item is not an object";
				return;
			}
			QJsonObject item = itemIt->toObject();
			setItemData(row, item);
		}
		if(!m_model->submit())
			qWarning() << "Could not submit after changing";
	}
	else if(operationString == "remove")
	{
		QJsonArray items = itemsIt->toArray();
		for(auto itemIt = items.begin(); itemIt != items.end(); ++itemIt)
		{
			int row = -1;
			if(itemIt->isString())
				row = getRowForKey(itemIt->toString());
			else if(itemIt->isDouble())
				row = getRowForKey(QString::number(itemIt->toDouble()));
			else
			{
				qWarning() << "Invalid key type for item" << *itemIt;
				continue;
			}
			if(row < 0)
			{
				qWarning() << "Row for" << itemIt->toString() << "not found";
				continue;
			}
			if(!m_model->removeRow(row))
				qWarning() << "Could not remove row";
		}
		if(!m_model->submit())
			qWarning() << "Could not submit after removing";
	}
	else if(operationString == "insert")
	{
		int row = m_model->rowCount();

		if(itemsIt->isObject())
		{
			QJsonObject items = itemsIt->toObject();
			if(m_model->insertRows(row, items.size()))
			{
				for(auto itemIt = items.begin(); itemIt != items.end(); ++itemIt)
				{
					QJsonObject item = itemIt->toObject();
					item.insert(keyName(), itemIt.key()); // Add key to item
					setItemData(row, item);
					row++;
				}
			}
			else
				qWarning() <<"Could not insert rows";
		}
		else if(itemsIt->isArray())
		{
			QJsonArray items = itemsIt->toArray();
			if(m_model->insertRows(row, items.size()))
			{
				for(auto itemIt = items.begin(); itemIt != items.end(); ++itemIt)
				{
					QJsonObject item = itemIt->toObject();
					setItemData(row, item);
					row++;
				}
			}
			else
				qWarning() <<"Could not insert rows from array";
		}
		else
			qWarning() << "Items to insert neither object nor array";
		if(!m_model->submit())
			qWarning() << "Could not submit after inserting";
	}
}

void JsonViewModel::setModel(QAbstractItemModel* model)
{
	if (m_model == model)
		return;

	if(m_model)
	{
		disconnect(m_model, &QAbstractItemModel::dataChanged, this, &JsonViewModel::dataChanged);
		disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &JsonViewModel::rowsAboutToBeRemoved);
		disconnect(m_model, &QAbstractItemModel::rowsInserted, this, &JsonViewModel::rowsInserted);
		disconnect(m_model, &QAbstractItemModel::modelReset, this, &JsonViewModel::modelReset);
		mRoleNames.clear();
		mHeaderData.clear();
		mKeyToRowCache.clear();
	}

	m_model = model;

	if(m_model)
	{
		connect(m_model, &QAbstractItemModel::dataChanged, this, &JsonViewModel::dataChanged);
		connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &JsonViewModel::rowsAboutToBeRemoved);
		connect(m_model, &QAbstractItemModel::rowsInserted, this, &JsonViewModel::rowsInserted);
		connect(m_model, &QAbstractItemModel::modelReset, this, &JsonViewModel::modelReset);
		mRoleNames = m_model->roleNames();
		int columnCount = m_model->columnCount();
		for(int i = 0; i < columnCount; ++i)
			mHeaderData[i] = m_model->headerData(i, Qt::Horizontal).toString();
	}

	Q_EMIT modelChanged(m_model);
}

void JsonViewModel::setKeyItem(int keyItem)
{
	if (mKeyItem == keyItem)
		return;

	mKeyItem = keyItem;
	Q_EMIT keyItemChanged(mKeyItem);
}

void JsonViewModel::setUseColumns(bool useColumns)
{
	if (mUseColumns == useColumns)
		return;

	mUseColumns = useColumns;
	Q_EMIT useColumnsChanged(mUseColumns);
}

void JsonViewModel::setUseRowBasedProtocol(bool useRowBasedProtocol)
{
	if (mUseRowBasedProtocol == useRowBasedProtocol)
		return;

	mUseRowBasedProtocol = useRowBasedProtocol;
	Q_EMIT useRowBasedProtocolChanged(mUseRowBasedProtocol);
}

void JsonViewModel::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	Q_UNUSED(roles);
	Q_ASSERT(m_model);

	QJsonObject outObject;
	if(mUseRowBasedProtocol)
	{
		const int first = topLeft.row();
		const int last = bottomRight.row();
		outObject.insert(QStringLiteral("operation"), QStringLiteral("rowDataChanged"));
		outObject.insert(QStringLiteral("items"), fetchRowsAsArray(first, last));
		outObject.insert(QStringLiteral("start"), first);
		outObject.insert(QStringLiteral("end"), last);
	}
	else
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("dataChanged"));
		outObject.insert(QStringLiteral("items"), fetchRows(topLeft.row(), bottomRight.row()));
	}
	QJsonDocument outDocument(outObject);
	sendMessage(outDocument);
}

void JsonViewModel::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
	Q_UNUSED(parent);
	Q_ASSERT(m_model);

	mKeyToRowCache.clear(); // TODO

	QJsonObject outObject;

	if(mUseRowBasedProtocol)
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("rowsRemoved"));
		outObject.insert(QStringLiteral("start"), start);
		outObject.insert(QStringLiteral("end"), end);
	}
	else
	{
		QJsonArray items;
		for(int i = start; i <= end; ++i)
		{
			QModelIndex index = m_model->index(i, 0);
			items.append(m_model->data(index, mKeyItem).toString());
		}

		outObject.insert(QStringLiteral("operation"), QStringLiteral("removed"));
		outObject.insert(QStringLiteral("items"), items);
	}

	QJsonDocument outDocument(outObject);
	sendMessage(outDocument);
}

void JsonViewModel::rowsInserted(const QModelIndex& parent, int start, int end)
{
	Q_UNUSED(parent);
	Q_ASSERT(m_model);

	mKeyToRowCache.clear(); // TODO

	QJsonObject outObject;
	if(mUseRowBasedProtocol)
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("rowsInserted"));
		outObject.insert(QStringLiteral("items"), fetchRowsAsArray(start, end));
		outObject.insert(QStringLiteral("start"), start);
		outObject.insert(QStringLiteral("end"), end);
	}
	else
	{
		outObject.insert(QStringLiteral("operation"), QStringLiteral("inserted"));
		outObject.insert(QStringLiteral("items"), fetchRows(start, end));
	}
	QJsonDocument outDocument(outObject);
	sendMessage(outDocument);
}

void JsonViewModel::modelReset()
{
	mKeyToRowCache.clear();
	mRowKeys.clear();
	mRowKeys.resize(m_model->rowCount());
	sendEntireData();
}

QJsonObject JsonViewModel::fetchRows(int start, int end)
{
	QJsonObject outData;
	for(int i = start; i <= end; ++i)
	{
		QString key;
		QJsonObject outValue;
		if(mUseColumns)
		{
			QModelIndex keyIndex = m_model->index(i, mKeyItem);
			QVariant keyValue = m_model->data(keyIndex);
			if(!keyValue.isValid())
			{
				continue; // Skip invalid keys
			}
			key = keyValue.toString();
			for(auto it = mHeaderData.begin(); it != mHeaderData.end(); ++it)
			{
				QModelIndex index = m_model->index(i, it.key());
				if(it.key() != mKeyItem)
					outValue.insert(it.value(), QJsonValue::fromVariant(m_model->data(index)));
			}
		}
		else
		{
			QModelIndex index = m_model->index(i, 0);
			QVariant keyValue = m_model->data(index, mKeyItem);
			if(!keyValue.isValid())
				continue; // Skip invalid keys
			key = keyValue.toString();
			outValue = fetchRowRoles(index);
		}
		outData.insert(key, outValue);
		mKeyToRowCache[key] = i;
	}

	return outData;
}

QJsonArray JsonViewModel::fetchRowsAsArray(int start, int end)
{
	QJsonArray out;
	for(int i = start; i <= end; ++i)
	{
		QJsonObject outValue;
		if(mUseColumns)
		{
			for(auto it = mHeaderData.begin(); it != mHeaderData.end(); ++it)
			{
				QModelIndex index = m_model->index(i, it.key());
				outValue.insert(it.value(), QJsonValue::fromVariant(m_model->data(index)));
			}
		}
		else
		{
			QModelIndex index = m_model->index(i, 0);
			outValue = fetchRowRoles(index, true);
		}
		out.append(outValue);
	}

	return out;
}

QJsonObject JsonViewModel::fetchRowRoles(const QModelIndex& index, bool includeKeyItem)
{
	Q_ASSERT(m_model);

	QJsonObject outValue;
	for(auto it = mRoleNames.begin(); it != mRoleNames.end(); ++it)
	{
		if(includeKeyItem || it.key() != mKeyItem)
			outValue.insert(it.value(), QJsonValue::fromVariant(m_model->data(index, it.key())));
	}

	return outValue;
}

void JsonViewModel::setItemData(int row, const QJsonObject& item)
{
	if(mUseColumns)
	{
		for(auto headerIt = mHeaderData.begin(); headerIt != mHeaderData.end(); ++headerIt)
		{
			if(item.contains(headerIt.value()) && headerIt.key() != mKeyItem)
			{
				QModelIndex index = m_model->index(row, headerIt.key());
				QVariant value = item[headerIt.value()].toVariant();
				m_model->setData(index, value);
			}
		}
	}
	else
	{
		for(auto roleIt = mRoleNames.begin(); roleIt != mRoleNames.end(); ++roleIt)
		{
			if(item.contains(roleIt.value()) && roleIt.key() != mKeyItem)
			{
				QModelIndex index = m_model->index(row, 0);
				QVariant value = item[roleIt.value()].toVariant();
				m_model->setData(index, value, roleIt.key());
			}
		}
	}
}

int JsonViewModel::getRowForKey(const QString& key)
{
	Q_ASSERT(m_model);

	if(mKeyToRowCache.contains(key))
		return mKeyToRowCache.value(key);
	else
	{
		qDebug() << "Row not found in Cache";
		QList<int> knownRows = mKeyToRowCache.values();
		int rowCount = m_model->rowCount();
		for(int i = 0; i < rowCount; ++i)
		{
			if(!knownRows.contains(i))
			{
				QString value;
				if(mUseColumns)
				{
					QModelIndex index = m_model->index(i, mKeyItem);
					value = m_model->data(index).toString();
				}
				else
				{
					QModelIndex index = m_model->index(i, 0);
					value = m_model->data(index, mKeyItem).toString();
				}
				qDebug() << value;
				mKeyToRowCache[value] = i;
				if(value == key)
					return i;
			}
		}
	}
	return -1; // Row not found
}

void JsonViewModel::sendMessage(const QJsonDocument& document)
{
	QByteArray data = document.toJson();
	Q_EMIT sendMessageAsByteArray(data);

	static const QMetaMethod sendMessageAsStringSignal = QMetaMethod::fromSignal(&JsonViewModel::sendMessageAsString);
	if(isSignalConnected(sendMessageAsStringSignal))
		Q_EMIT sendMessageAsString(QString::fromUtf8(data));
}

} // namespace qtmodelserver
