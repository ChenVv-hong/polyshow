#pragma once

#include "core/LayerEditing.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

class QLocalServer;
class QLocalSocket;

namespace PolyShow
{

/// Carries one validated IPC write request from the worker thread to the GUI thread.
struct IpcPrimitiveWriteMessage
{
    /// Internal sequence used to route one response back to its socket.
    quint64 sequence {0};

    /// Optional caller-provided request identifier.
    QString request_id;

    /// Target layer name matched against the runtime document.
    QString layer_name;

    /// Parsed primitive payload.
    PrimitiveWriteRequest request;
};

/// Carries one GUI-thread write result back to the worker thread.
struct IpcPrimitiveWriteResult
{
    /// Internal sequence used to route one response back to its socket.
    quint64 sequence {0};

    /// Optional caller-provided request identifier.
    QString request_id;

    /// Whether the write succeeded.
    bool ok {false};

    /// Human-readable result text returned to the caller.
    QString message;

    /// Target layer name echoed back to the caller.
    QString layer_name;

    /// Optional primitive name echoed back on named writes.
    QString primitive_name;

    /// Result classification for successful writes.
    QString result;
};

/// Owns the blocking local-socket server and translates line-delimited JSON into write requests.
class IpcListenerWorker final : public QObject
{
    Q_OBJECT

public:
    /// Creates one worker instance that lives entirely on the IPC thread.
    explicit IpcListenerWorker(QObject *parent = nullptr);

    /// Stops any active server and releases connected sockets.
    ~IpcListenerWorker() override;

    /// Starts listening on the fixed `polyshow-ipc` local socket service.
    [[nodiscard]]
    bool startListening(QString *errorMessage = nullptr);

    /// Returns the currently active or configured IPC endpoint address.
    [[nodiscard]]
    QString listeningAddress() const;

    /// Stops listening and disconnects every active client.
    void stopListening();

signals:
    /// Emitted after one request passes protocol validation and needs a GUI-thread write.
    void primitiveWriteRequested(const PolyShow::IpcPrimitiveWriteMessage &message);

    /// Emitted when the worker rejects one request before it reaches the GUI thread.
    void protocolErrorLogged(const QString &message);

public slots:
    /// Sends one GUI-thread write result back to the originating client connection.
    void deliverWriteResult(const PolyShow::IpcPrimitiveWriteResult &result);

private slots:
    /// Accepts pending socket connections from the local server.
    void acceptPendingConnections();

    /// Parses newline-delimited request messages from one active socket.
    void readSocketMessages();

    /// Cleans up one socket after it disconnects.
    void handleSocketDisconnected();

    /// Logs one transport-level socket error.
    void handleSocketError();

private:
    QLocalServer *m_server {nullptr};
    QHash<QLocalSocket *, QByteArray> m_socket_buffers;
    QHash<quint64, QPointer<QLocalSocket>> m_pending_requests;
    quint64 m_next_sequence {1};
};

} // namespace PolyShow

Q_DECLARE_METATYPE(PolyShow::IpcPrimitiveWriteMessage)
Q_DECLARE_METATYPE(PolyShow::IpcPrimitiveWriteResult)
