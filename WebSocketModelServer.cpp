/* WebSocketModelServer.cpp

BSD 2-Clause License

Copyright (c) 2018-2020, Fabian Herb
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
