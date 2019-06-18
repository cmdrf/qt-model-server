#include "WebSocketModelServer.h"
#include <QWebSocketServer>
#include <QWebSocket>

namespace qtmodelserver
{

WebSocketModelServer::WebSocketModelServer(QObject* parent) :
	QObject(parent),
	mWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"), QWebSocketServer::NonSecureMode, this))
{

}

WebSocketModelServer::~WebSocketModelServer()
{
	mWebSocketServer->close();
	qDeleteAll(m_clients.begin(), m_clients.end());
	qDeleteAll(mModels.begin(), mModels.end());
}

void WebSocketModelServer::setModel(QAbstractItemModel* model, int keyRole, const QString& path, bool useColumns)
{
	Q_ASSERT(!mWebSocketServer->isListening());

	JsonViewModel* m = new JsonViewModel(this);
	m->setModel(model);
	m->setKeyItem(keyRole);
	m->setUseColumns(useColumns);
	if(mModels.contains(path))
		delete mModels.value(path);
	mModels[path] = m;
}

void WebSocketModelServer::listen(quint16 port)
{
	if (mWebSocketServer->listen(QHostAddress::Any, port))
	{
		connect(mWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketModelServer::onNewConnection);
//		connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketViewModel::closed);
	}
	else
		qWarning() << "listen() failed:" << mWebSocketServer->errorString();
}

void WebSocketModelServer::onNewConnection()
{
	QWebSocket* socket = mWebSocketServer->nextPendingConnection();
	auto path = socket->requestUrl().path();

	if(mModels.contains(path))
	{
		JsonViewModel* model = mModels.value(path);
		Q_ASSERT(model);
		connect(socket, &QWebSocket::disconnected, this, &WebSocketModelServer::socketDisconnected);
		connect(socket, SIGNAL(textMessageReceived(const QString&)), model, SLOT(receiveMessage(const QString&)));

		// Hack because QWebSocket has no slot for sending messages:
		auto msgConnection = connect(model, &JsonViewModel::sendMessageAsString, [socket](const QString& msg){socket->sendTextMessage(msg);});
		// Disconnecting is not done automatically, so do it manually:
		connect(socket, &QObject::destroyed, [msgConnection](QObject*){disconnect(msgConnection);});

		m_clients << socket;

		model->sendEntireData();
	}
	else
	{
		qWarning() << "Request to unknown path" << path;
		socket->close();
		socket->deleteLater();
	}
}

void WebSocketModelServer::socketDisconnected()
{
	qDebug() << "socketDisconnected()";
	QWebSocket* client = qobject_cast<QWebSocket*>(sender());
	if(client)
	{
		m_clients.removeAll(client);
		client->deleteLater();
	}
}

} // namespace qtmodelserver
