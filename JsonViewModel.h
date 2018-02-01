#ifndef JSONVIEWMODEL_H
#define JSONVIEWMODEL_H

#include <QObject>
#include <QVector>
#include <QHash>

class QAbstractItemModel;

/// Provides a JSON message interface to a QAbstractItemModel
/** Set the model property for the QAbstractItemModel side. Connect
	sendMessage() signal and receiveMessage() slot for the JSON side. */
class JsonViewModel : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)

	/// Item to use as unique key
	/** Role or column.
		@see keyItem()
		@see setKeyItem() */
	Q_PROPERTY(int keyItem READ keyItem WRITE setKeyItem NOTIFY keyItemChanged)

	/// Use multiple columns with a single role
	/** When false use multiple roles with a single column (QML style). */
	Q_PROPERTY(bool useColumns READ useColumns WRITE setUseColumns NOTIFY useColumnsChanged)

public:
	explicit JsonViewModel(QObject* parent = nullptr);

	QAbstractItemModel* model() const {return m_model;}

	int keyItem() const {return mKeyItem;}

	bool useColumns() const {return mUseColumns;}

signals:
	/// Send message to client
	/** QString variant.
		@see sendMessageAsByteArray() */
	void sendMessageAsString(const QString& message);

	/// Send message to client
	/** QByteArray variant. Prefer this over QString since QByteArray is the native format
		of the Qt JSON serializer.
		@see sendMessageAsString() */
	void sendMessageAsByteArray(const QByteArray& message);

	void modelChanged(QAbstractItemModel* model);

	void keyItemChanged(int keyItem);

	void useColumnsChanged(bool useColumns);

public slots:
	/// Send entire model data as a JSON message
	/** Call this e.g. when a new client connects. */
	void sendEntireData();

	/// Handle JSON message from client
	/** QString overload. */
	void receiveMessage(const QString& message);

	/// Handle JSON message from client
	/** QByteArray overload. Prefer this over QString since QByteArray is the native format
		of the Qt JSON serializer.*/
	void receiveMessage(const QByteArray& message);

	void setModel(QAbstractItemModel* model);

	void setKeyItem(int keyItem);

	void setUseColumns(bool useColumns);

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

	void sendMessage(const QJsonDocument& document);

	QAbstractItemModel* m_model = nullptr;

	QHash<int, QByteArray> mRoleNames;
	QHash<int, QString> mHeaderData;
	QHash<QString, int> mKeyToRowCache;
	int mKeyItem = 0;
	bool mUseColumns = false;
};

#endif // JSONVIEWMODEL_H
