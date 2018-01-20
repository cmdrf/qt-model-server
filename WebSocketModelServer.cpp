#include "WebSocketModelServer.h"
#include <QWebSocketServer>
#include <QWebSocket>

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
		qWarning() << "listen() failed:" << mWebSocketServer->error();
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
		connect(socket, &QWebSocket::textMessageReceived, model, &JsonViewModel::receiveMessage);

		// Hack because QWebSocket has no slot for sending messages:
		auto msgConnection = connect(model, &JsonViewModel::sendMessage, [socket](const QString& msg){socket->sendTextMessage(msg);});
		// Disconnecting is not done automatically, so do it manually:
		connect(socket, &QObject::destroyed, [msgConnection](QObject*){disconnect(msgConnection);});

		m_clients << socket;

		socket->sendTextMessage(model->entireData());
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
