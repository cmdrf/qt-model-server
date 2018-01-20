#ifndef JSONMODELVIEW_H
#define JSONMODELVIEW_H

#include <QObject>
#include <QVector>
#include <QHash>

class QAbstractItemModel;

class JsonViewModel : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)

	/** Role or column. */
	Q_PROPERTY(int keyItem READ keyItem WRITE setKeyItem NOTIFY keyItemChanged)

	Q_PROPERTY(bool useColumns READ useColumns WRITE setUseColumns NOTIFY useColumnsChanged)

public:
	explicit JsonViewModel(QObject* parent = nullptr);

	QAbstractItemModel* model() const {return m_model;}

	int keyItem() const {return mKeyItem;}

	QString entireData();

	bool useColumns() const {return m_useColumns;}

signals:
	void sendMessage(const QString& message);

	void modelChanged(QAbstractItemModel* model);

	void keyItemChanged(int keyItem);

	void useColumnsChanged(bool useColumns);

public slots:
	void sendEntireData();
	void receiveMessage(const QString& message);

	void setModel(QAbstractItemModel* model);

	void setKeyItem(int keyItem);

	void setUseColumns(bool useColumns)
	{
		if (m_useColumns == useColumns)
			return;

		m_useColumns = useColumns;
		emit useColumnsChanged(m_useColumns);
	}

protected slots:
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());
	void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
	void rowsInserted(const QModelIndex& parent, int start, int end);
	void modelReset();

private:
	QJsonObject fetchRows(int start, int end);
	QJsonObject fetchRowRoles(const QModelIndex& index);

	/** @see mKeyToRowCache */
	int getRowForKey(const QString& key);

	QAbstractItemModel* m_model = nullptr;

	QHash<int, QByteArray> mRoleNames;
	QHash<int, QString> mHeaderData;
	QHash<QString, int> mKeyToRowCache;
	int mKeyItem = 0;
	bool m_useColumns;
};

#endif // JSONMODELVIEW_H
