/* WebSocketModelServer.h

BSD 2-Clause License

Copyright (c) 2018-2021, Fabian Herb
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

	void setVariantToJsonValueFunction(std::function<QJsonValue (const QVariant&)> variantToJsonValueFunction) {mVariantToJsonValueFunction = variantToJsonValueFunction;}
	void setJsonValueToVariantFunction(std::function<QVariant (const QJsonValue&)> jsonValueToVariantFunction) {mJsonValueToVariantFunction = jsonValueToVariantFunction;}

Q_SIGNALS:

public Q_SLOTS:

protected Q_SLOTS:
	void onNewConnection();
	void socketDisconnected();

private:
	QWebSocketServer* mWebSocketServer;
	QMap<QString, JsonViewModel*> mModels;
	QList<QWebSocket*> m_clients;

	std::function<QJsonValue (const QVariant&)> mVariantToJsonValueFunction;
	std::function<QVariant (const QJsonValue&)> mJsonValueToVariantFunction;
};

}

#endif // QTMODELSERVER_WEBSOCKETMODELSERVER_H
