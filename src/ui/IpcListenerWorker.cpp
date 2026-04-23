#include "ui/IpcListenerWorker.h"

#include "core/PrimitiveEditing.h"

#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>

namespace PolyShow
{

namespace
{

constexpr quint64 kInitialSequence = 1;

QString configuredListenName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("polyshow-ipc");
#else
    return QStringLiteral("/tmp/polyshow-ipc.sock");
#endif
}

QString configuredNativeEndpoint()
{
#ifdef Q_OS_WIN
    return QStringLiteral(R"(\\.\pipe\polyshow-ipc)");
#else
    return QStringLiteral("/tmp/polyshow-ipc.sock");
#endif
}

QString optionalStringField(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : QString();
}

bool parseOptionalStringField(const QJsonObject &object, const QString &key, QString *value, QString *errorMessage)
{
    const QJsonValue fieldValue = object.value(key);
    if (fieldValue.isUndefined())
    {
        if (value != nullptr)
        {
            value->clear();
        }
        return true;
    }

    if (!fieldValue.isString())
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Field \"%1\" must be a string when present.").arg(key);
        }
        return false;
    }

    if (value != nullptr)
    {
        *value = fieldValue.toString().trimmed();
    }
    return true;
}

bool parseRequiredStringField(const QJsonObject &object, const QString &key, QString *value, QString *errorMessage)
{
    const QJsonValue fieldValue = object.value(key);
    if (!fieldValue.isString())
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Field \"%1\" must be a string.").arg(key);
        }
        return false;
    }

    const QString text = fieldValue.toString().trimmed();
    if (text.isEmpty())
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Field \"%1\" cannot be empty.").arg(key);
        }
        return false;
    }

    if (value != nullptr)
    {
        *value = text;
    }
    return true;
}

bool parsePointArray(const QJsonArray &pointsArray, QVector<Point2D> *points, QString *errorMessage)
{
    QVector<Point2D> parsedPoints;
    parsedPoints.reserve(pointsArray.size());

    for (int index = 0; index < pointsArray.size(); ++index)
    {
        const QJsonValue pointValue = pointsArray.at(index);
        if (!pointValue.isObject())
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("primitive.points[%1] must be an object.").arg(index);
            }
            return false;
        }

        const QJsonObject pointObject = pointValue.toObject();
        const QJsonValue xValue = pointObject.value(QStringLiteral("x"));
        const QJsonValue yValue = pointObject.value(QStringLiteral("y"));
        if (!xValue.isDouble() || !yValue.isDouble())
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("primitive.points[%1] requires numeric x and y.").arg(index);
            }
            return false;
        }

        parsedPoints.append(Point2D {xValue.toDouble(), yValue.toDouble()});
    }

    if (points != nullptr)
    {
        *points = parsedPoints;
    }
    return true;
}

bool parsePrimitiveKindText(const QString &kindText, PrimitiveKind *kind, QString *errorMessage)
{
    if (kindText == QStringLiteral("point"))
    {
        if (kind != nullptr)
        {
            *kind = PrimitiveKind::Point;
        }
        return true;
    }

    if (kindText == QStringLiteral("polyline"))
    {
        if (kind != nullptr)
        {
            *kind = PrimitiveKind::Polyline;
        }
        return true;
    }

    if (kindText == QStringLiteral("polygon"))
    {
        if (kind != nullptr)
        {
            *kind = PrimitiveKind::Polygon;
        }
        return true;
    }

    if (errorMessage != nullptr)
    {
        *errorMessage = QStringLiteral("primitive.kind must be point, polyline, or polygon.");
    }
    return false;
}

bool parseStyleObject(const QJsonObject &styleObject, PrimitiveStyle *style, QString *errorMessage)
{
    if (style == nullptr)
    {
        return false;
    }

    PrimitiveStyle parsedStyle = *style;

    const QJsonValue colorValue = styleObject.value(QStringLiteral("color"));
    if (!colorValue.isUndefined())
    {
        if (!colorValue.isString() || !parseColorText(colorValue.toString(), parsedStyle.color))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("style.color must use #RRGGBB or #RRGGBBAA.");
            }
            return false;
        }
    }

    const QJsonValue fillColorValue = styleObject.value(QStringLiteral("fillColor"));
    if (!fillColorValue.isUndefined())
    {
        if (!fillColorValue.isString() || !parseColorText(fillColorValue.toString(), parsedStyle.fill_color))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("style.fillColor must use #RRGGBB or #RRGGBBAA.");
            }
            return false;
        }
    }

    const QJsonValue fillEnabledValue = styleObject.value(QStringLiteral("fillEnabled"));
    if (!fillEnabledValue.isUndefined())
    {
        if (!fillEnabledValue.isBool())
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("style.fillEnabled must be a boolean.");
            }
            return false;
        }

        parsedStyle.fill_enabled = fillEnabledValue.toBool();
    }

    const QJsonValue widthValue = styleObject.value(QStringLiteral("width"));
    if (!widthValue.isUndefined())
    {
        if (!widthValue.isDouble() || widthValue.toDouble() <= 0.0)
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("style.width must be greater than 0.");
            }
            return false;
        }

        parsedStyle.width = widthValue.toDouble();
    }

    const QJsonValue pointSizeValue = styleObject.value(QStringLiteral("pointSize"));
    if (!pointSizeValue.isUndefined())
    {
        if (!pointSizeValue.isDouble() || pointSizeValue.toDouble() <= 0.0)
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("style.pointSize must be greater than 0.");
            }
            return false;
        }

        parsedStyle.point_size = pointSizeValue.toDouble();
    }

    *style = parsedStyle;
    return true;
}

QJsonObject resultToJsonObject(const IpcPrimitiveWriteResult &result)
{
    QJsonObject responseObject;
    if (!result.request_id.isEmpty())
    {
        responseObject.insert(QStringLiteral("requestId"), result.request_id);
    }

    responseObject.insert(QStringLiteral("ok"), result.ok);
    responseObject.insert(QStringLiteral("message"), result.message);
    responseObject.insert(QStringLiteral("layerName"), result.layer_name);

    if (!result.primitive_name.isEmpty())
    {
        responseObject.insert(QStringLiteral("primitiveName"), result.primitive_name);
    }

    if (result.ok && !result.result.isEmpty())
    {
        responseObject.insert(QStringLiteral("result"), result.result);
    }

    return responseObject;
}

} // namespace

IpcListenerWorker::IpcListenerWorker(QObject *parent)
    : QObject(parent)
{
}

IpcListenerWorker::~IpcListenerWorker()
{
    stopListening();
}

bool IpcListenerWorker::startListening(QString *errorMessage)
{
    if (m_server == nullptr)
    {
        m_server = new QLocalServer(this);
        connect(m_server, &QLocalServer::newConnection, this, &IpcListenerWorker::acceptPendingConnections);
    }

    if (m_server->isListening())
    {
        return true;
    }

#ifndef Q_OS_WIN
    QLocalServer::removeServer(configuredListenName());
#endif

    const QString listenName = configuredListenName();
    if (m_server->listen(listenName))
    {
        return true;
    }

    if (m_server->serverError() == QAbstractSocket::AddressInUseError)
    {
        QLocalServer::removeServer(listenName);
        if (m_server->listen(listenName))
        {
            return true;
        }
    }

    if (errorMessage != nullptr)
    {
        *errorMessage = QStringLiteral("Failed to listen on %1: %2")
                            .arg(configuredNativeEndpoint(), m_server->errorString());
    }
    return false;
}

QString IpcListenerWorker::listeningAddress() const
{
    if (m_server != nullptr && m_server->isListening())
    {
        const QString fullServerName = m_server->fullServerName();
        if (!fullServerName.isEmpty())
        {
            return fullServerName;
        }
    }

    return configuredNativeEndpoint();
}

void IpcListenerWorker::stopListening()
{
    const QList<QLocalSocket *> sockets = m_socket_buffers.keys();
    for (QLocalSocket *socket : sockets)
    {
        if (socket == nullptr)
        {
            continue;
        }

        m_socket_buffers.remove(socket);
        for (auto it = m_pending_requests.begin(); it != m_pending_requests.end();)
        {
            if (it.value() == socket)
            {
                it = m_pending_requests.erase(it);
            }
            else
            {
                ++it;
            }
        }

        socket->disconnect(this);
        socket->abort();
        delete socket;
    }

    m_pending_requests.clear();
    m_next_sequence = kInitialSequence;

    if (m_server != nullptr)
    {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }

#ifndef Q_OS_WIN
    QLocalServer::removeServer(configuredListenName());
#endif
}

void IpcListenerWorker::deliverWriteResult(const IpcPrimitiveWriteResult &result)
{
    QPointer<QLocalSocket> socket = m_pending_requests.take(result.sequence);
    if (socket.isNull())
    {
        return;
    }

    const QByteArray payload = QJsonDocument(resultToJsonObject(result)).toJson(QJsonDocument::Compact) + '\n';
    socket->write(payload);
    socket->flush();
}

void IpcListenerWorker::acceptPendingConnections()
{
    if (m_server == nullptr)
    {
        return;
    }

    while (m_server->hasPendingConnections())
    {
        QLocalSocket *socket = m_server->nextPendingConnection();
        if (socket == nullptr)
        {
            continue;
        }

        m_socket_buffers.insert(socket, QByteArray());
        connect(socket, &QLocalSocket::readyRead, this, &IpcListenerWorker::readSocketMessages);
        connect(socket, &QLocalSocket::disconnected, this, &IpcListenerWorker::handleSocketDisconnected);
        connect(socket, &QLocalSocket::errorOccurred, this, &IpcListenerWorker::handleSocketError);
    }
}

void IpcListenerWorker::readSocketMessages()
{
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == nullptr)
    {
        return;
    }

    QByteArray &buffer = m_socket_buffers[socket];
    buffer.append(socket->readAll());

    while (true)
    {
        const int newlineIndex = buffer.indexOf('\n');
        if (newlineIndex < 0)
        {
            break;
        }

        QByteArray line = buffer.left(newlineIndex);
        buffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty() && line.endsWith('\r'))
        {
            line.chop(1);
        }

        const QByteArray trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty())
        {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(trimmedLine, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            const QString message = parseError.error == QJsonParseError::NoError
                ? QStringLiteral("Request body must be a JSON object.")
                : QStringLiteral("Invalid JSON: %1").arg(parseError.errorString());
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(message));

            IpcPrimitiveWriteResult errorResult;
            errorResult.ok = false;
            errorResult.message = message;
            errorResult.layer_name = QString();
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        const QJsonObject rootObject = document.object();
        QString requestId;
        QString layerName;
        QString errorMessage;
        if (!parseOptionalStringField(rootObject, QStringLiteral("requestId"), &requestId, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = QString();
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        if (!parseOptionalStringField(rootObject, QStringLiteral("layerName"), &layerName, nullptr))
        {
            layerName.clear();
        }
        QString action;
        if (!parseRequiredStringField(rootObject, QStringLiteral("action"), &action, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = layerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        if (action != QStringLiteral("write_primitive"))
        {
            errorMessage = QStringLiteral("Unknown action \"%1\".").arg(action);
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = layerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        QString targetLayerName;
        if (!parseRequiredStringField(rootObject, QStringLiteral("layerName"), &targetLayerName, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = layerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        const QJsonValue primitiveValue = rootObject.value(QStringLiteral("primitive"));
        if (!primitiveValue.isObject())
        {
            errorMessage = QStringLiteral("Field \"primitive\" must be an object.");
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        const QJsonObject primitiveObject = primitiveValue.toObject();
        QString kindText;
        if (!parseRequiredStringField(primitiveObject, QStringLiteral("kind"), &kindText, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        PrimitiveKind kind = PrimitiveKind::Point;
        if (!parsePrimitiveKindText(kindText, &kind, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        const QJsonValue pointsValue = primitiveObject.value(QStringLiteral("points"));
        if (!pointsValue.isArray())
        {
            errorMessage = QStringLiteral("primitive.points must be an array.");
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        QVector<Point2D> points;
        if (!parsePointArray(pointsValue.toArray(), &points, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        if (!validatePrimitivePoints(kind, points, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }

        PrimitiveStyle style;
        const QJsonValue styleValue = primitiveObject.value(QStringLiteral("style"));
        if (!styleValue.isUndefined())
        {
            if (!styleValue.isObject())
            {
                errorMessage = QStringLiteral("primitive.style must be an object when present.");
                emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

                IpcPrimitiveWriteResult errorResult;
                errorResult.request_id = requestId;
                errorResult.ok = false;
                errorResult.message = errorMessage;
                errorResult.layer_name = targetLayerName;
                socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
                socket->flush();
                continue;
            }

            if (!parseStyleObject(styleValue.toObject(), &style, &errorMessage))
            {
                emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

                IpcPrimitiveWriteResult errorResult;
                errorResult.request_id = requestId;
                errorResult.ok = false;
                errorResult.message = errorMessage;
                errorResult.layer_name = targetLayerName;
                socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
                socket->flush();
                continue;
            }
        }

        IpcPrimitiveWriteMessage writeMessage;
        writeMessage.sequence = m_next_sequence++;
        writeMessage.request_id = requestId;
        writeMessage.layer_name = targetLayerName;
        writeMessage.request.kind = kind;
        writeMessage.request.points = points;
        writeMessage.request.style = style;
        if (!parseOptionalStringField(primitiveObject, QStringLiteral("name"), &writeMessage.request.name, &errorMessage))
        {
            emit protocolErrorLogged(QStringLiteral("[error] IPC request rejected: %1").arg(errorMessage));

            IpcPrimitiveWriteResult errorResult;
            errorResult.request_id = requestId;
            errorResult.ok = false;
            errorResult.message = errorMessage;
            errorResult.layer_name = targetLayerName;
            socket->write(QJsonDocument(resultToJsonObject(errorResult)).toJson(QJsonDocument::Compact) + '\n');
            socket->flush();
            continue;
        }
        writeMessage.request.visible = true;

        m_pending_requests.insert(writeMessage.sequence, socket);
        emit primitiveWriteRequested(writeMessage);
    }
}

void IpcListenerWorker::handleSocketDisconnected()
{
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == nullptr)
    {
        return;
    }

    m_socket_buffers.remove(socket);
    for (auto it = m_pending_requests.begin(); it != m_pending_requests.end();)
    {
        if (it.value() == socket)
        {
            it = m_pending_requests.erase(it);
        }
        else
        {
            ++it;
        }
    }

    socket->deleteLater();
}

void IpcListenerWorker::handleSocketError()
{
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == nullptr)
    {
        return;
    }

    emit protocolErrorLogged(
        QStringLiteral("[error] IPC socket error from %1: %2").arg(configuredNativeEndpoint(), socket->errorString()));
}

} // namespace PolyShow
