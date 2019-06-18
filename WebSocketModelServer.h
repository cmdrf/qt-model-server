#ifndef QTMODELSERVER_WEBSOCKETMODELSERVER_H
#define QTMODELSERVER_WEBSOCKETMODELSERVER_H

#include "JsonViewModel.h"
#include <QObject>
#include <QMap>

class QWebSocketServer;
class QWebSocket;

namespace qtmodelserver
{

class WebSocketModelServer : public QObject
{
	Q_OBJECT
public:
	explicit WebSocketModelServer(QObject* parent = nullptr);
	~WebSocketModelServer();

	/// Add a model to serve
	/** Mutiple models can be served by setting a different path for each.
		All models must be registered before calling listen()! */
	void setModel(QAbstractItemModel* model, int keyRole, const QString& path = "/", bool useColumns = false);

	void listen(quint16 port);

Q_SIGNALS:

public Q_SLOTS:

protected Q_SLOTS:
	void onNewConnection();
	void socketDisconnected();

private:
	QWebSocketServer* mWebSocketServer;
	QMap<QString, JsonViewModel*> mModels;
	QList<QWebSocket*> m_clients;
};

}

#endif // QTMODELSERVER_WEBSOCKETMODELSERVER_H
