#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iomanip>
#include <limits>
#include <locale>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace PolyShow::Ipc
{

inline constexpr char kServerName[] = "polyshow-ipc";
inline constexpr char kWindowsNativeEndpoint[] = R"(\\.\pipe\polyshow-ipc)";
inline constexpr char kUnixNativeEndpoint[] = "/tmp/polyshow-ipc.sock";

[[nodiscard]]
inline std::string nativeEndpoint()
{
#ifdef _WIN32
    return std::string(kWindowsNativeEndpoint);
#else
    return std::string(kUnixNativeEndpoint);
#endif
}

struct Point
{
    double x {0.0};
    double y {0.0};
};

enum class PrimitiveKind
{
    Point,
    Polyline,
    Polygon
};

enum class QueueMode
{
    BoundedBlocking,
    Unbounded
};

struct PrimitiveStyle
{
    std::optional<std::string> color;
    std::optional<std::string> fillColor;
    std::optional<bool> fillEnabled;
    std::optional<double> width;
    std::optional<double> pointSize;

    [[nodiscard]]
    bool hasAnyValue() const
    {
        return color.has_value() || fillColor.has_value() || fillEnabled.has_value() || width.has_value()
            || pointSize.has_value();
    }
};

struct PrimitiveMessage
{
    std::string layerName;
    PrimitiveKind kind {PrimitiveKind::Point};
    std::vector<Point> points;
    std::string name;
    std::optional<PrimitiveStyle> style;
    std::string requestId;
};

struct ClientConfig
{
    QueueMode queueMode {QueueMode::BoundedBlocking};
    std::size_t queueCapacity {4096};
    std::string endpoint {nativeEndpoint()};
};

struct ClientStats
{
    std::uint64_t enqueued {0};
    std::uint64_t sent {0};
    std::uint64_t dropped {0};
    std::uint64_t connectFailures {0};
    std::uint64_t writeFailures {0};
    std::uint64_t responseFailures {0};
};

namespace Detail
{

inline constexpr std::chrono::milliseconds kConnectTimeout {250};
inline constexpr std::chrono::milliseconds kResponseTimeout {1000};

[[nodiscard]]
inline bool isValidColorText(const std::string &value)
{
    if (value.size() != 7 && value.size() != 9)
    {
        return false;
    }

    if (value.front() != '#')
    {
        return false;
    }

    for (std::size_t index = 1; index < value.size(); ++index)
    {
        const char ch = value[index];
        const bool isHexDigit = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
        if (!isHexDigit)
        {
            return false;
        }
    }

    return true;
}

[[nodiscard]]
inline std::string escapeJsonString(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (const unsigned char ch : value)
    {
        switch (ch)
        {
        case '\"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (ch < 0x20)
            {
                std::ostringstream stream;
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                escaped += stream.str();
            }
            else
            {
                escaped.push_back(static_cast<char>(ch));
            }
            break;
        }
    }

    return escaped;
}

[[nodiscard]]
inline std::string formatDouble(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(15) << value;
    return stream.str();
}

[[nodiscard]]
inline const char *primitiveKindText(PrimitiveKind kind)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return "point";
    case PrimitiveKind::Polyline:
        return "polyline";
    case PrimitiveKind::Polygon:
        return "polygon";
    default:
        return "point";
    }
}

inline bool validatePrimitiveMessage(const PrimitiveMessage &message)
{
    if (message.layerName.empty())
    {
        return false;
    }

    switch (message.kind)
    {
    case PrimitiveKind::Point:
        if (message.points.size() != 1)
        {
            return false;
        }
        break;
    case PrimitiveKind::Polyline:
        if (message.points.size() < 2)
        {
            return false;
        }
        break;
    case PrimitiveKind::Polygon:
        if (message.points.size() < 3)
        {
            return false;
        }
        break;
    default:
        return false;
    }

    if (!message.style.has_value())
    {
        return true;
    }

    const PrimitiveStyle &style = *message.style;
    if (style.color.has_value() && !isValidColorText(*style.color))
    {
        return false;
    }
    if (style.fillColor.has_value() && !isValidColorText(*style.fillColor))
    {
        return false;
    }
    if (style.width.has_value() && *style.width <= 0.0)
    {
        return false;
    }
    if (style.pointSize.has_value() && *style.pointSize <= 0.0)
    {
        return false;
    }

    return true;
}

[[nodiscard]]
inline std::string serializeMessageJson(const PrimitiveMessage &message)
{
    std::string json = "{";
    if (!message.requestId.empty())
    {
        json += "\"requestId\":\"" + escapeJsonString(message.requestId) + "\",";
    }

    json += "\"action\":\"write_primitive\",";
    json += "\"layerName\":\"" + escapeJsonString(message.layerName) + "\",";
    json += "\"primitive\":{";
    json += "\"kind\":\"";
    json += primitiveKindText(message.kind);
    json += "\"";

    if (!message.name.empty())
    {
        json += ",\"name\":\"" + escapeJsonString(message.name) + "\"";
    }

    json += ",\"points\":[";
    for (std::size_t index = 0; index < message.points.size(); ++index)
    {
        if (index > 0)
        {
            json += ",";
        }

        json += "{\"x\":";
        json += formatDouble(message.points[index].x);
        json += ",\"y\":";
        json += formatDouble(message.points[index].y);
        json += "}";
    }
    json += "]";

    if (message.style.has_value() && message.style->hasAnyValue())
    {
        json += ",\"style\":{";
        bool firstStyleField = true;
        const auto appendFieldSeparator = [&json, &firstStyleField]() {
            if (!firstStyleField)
            {
                json += ",";
            }
            firstStyleField = false;
        };

        if (message.style->color.has_value())
        {
            appendFieldSeparator();
            json += "\"color\":\"" + escapeJsonString(*message.style->color) + "\"";
        }
        if (message.style->fillColor.has_value())
        {
            appendFieldSeparator();
            json += "\"fillColor\":\"" + escapeJsonString(*message.style->fillColor) + "\"";
        }
        if (message.style->fillEnabled.has_value())
        {
            appendFieldSeparator();
            json += "\"fillEnabled\":";
            json += *message.style->fillEnabled ? "true" : "false";
        }
        if (message.style->width.has_value())
        {
            appendFieldSeparator();
            json += "\"width\":";
            json += formatDouble(*message.style->width);
        }
        if (message.style->pointSize.has_value())
        {
            appendFieldSeparator();
            json += "\"pointSize\":";
            json += formatDouble(*message.style->pointSize);
        }

        json += "}";
    }

    json += "}}\n";
    return json;
}

inline bool parseResponseOkField(const std::string &json, bool *ok)
{
    const std::size_t keyPosition = json.find("\"ok\"");
    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition = json.find(':', keyPosition + 4);
    if (colonPosition == std::string::npos)
    {
        return false;
    }

    std::size_t valuePosition = colonPosition + 1;
    while (valuePosition < json.size()
        && (json[valuePosition] == ' ' || json[valuePosition] == '\t' || json[valuePosition] == '\r'
            || json[valuePosition] == '\n'))
    {
        ++valuePosition;
    }

    if (json.compare(valuePosition, 4, "true") == 0)
    {
        if (ok != nullptr)
        {
            *ok = true;
        }
        return true;
    }

    if (json.compare(valuePosition, 5, "false") == 0)
    {
        if (ok != nullptr)
        {
            *ok = false;
        }
        return true;
    }

    return false;
}

#ifdef _WIN32

[[nodiscard]]
inline std::wstring utf8ToWide(const std::string &value)
{
    if (value.empty())
    {
        return {};
    }

    const int wideLength = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (wideLength <= 0)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::wstring wideString(static_cast<std::size_t>(wideLength), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wideString.data(), wideLength);
    wideString.pop_back();
    return wideString;
}

class NativeConnection final
{
public:
    NativeConnection() = default;

    ~NativeConnection()
    {
        close();
    }

    [[nodiscard]]
    bool isOpen() const
    {
        return m_handle != INVALID_HANDLE_VALUE;
    }

    void close()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }

        m_readBuffer.clear();
    }

    [[nodiscard]]
    bool connect(const std::string &endpoint, std::chrono::milliseconds timeout)
    {
        close();

        const std::wstring wideEndpoint = utf8ToWide(endpoint);
        const DWORD waitMilliseconds = timeout.count() > static_cast<long long>(std::numeric_limits<DWORD>::max())
            ? std::numeric_limits<DWORD>::max()
            : static_cast<DWORD>(timeout.count());

        if (!WaitNamedPipeW(wideEndpoint.c_str(), waitMilliseconds))
        {
            return false;
        }

        m_handle = CreateFileW(
            wideEndpoint.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (m_handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD readMode = PIPE_READMODE_BYTE;
        SetNamedPipeHandleState(m_handle, &readMode, nullptr, nullptr);
        return true;
    }

    [[nodiscard]]
    bool writeAll(const std::string &payload)
    {
        const char *data = payload.data();
        std::size_t remaining = payload.size();

        while (remaining > 0)
        {
            DWORD bytesWritten = 0;
            if (!WriteFile(
                    m_handle,
                    data,
                    static_cast<DWORD>(remaining > static_cast<std::size_t>(std::numeric_limits<DWORD>::max())
                            ? std::numeric_limits<DWORD>::max()
                            : remaining),
                    &bytesWritten,
                    nullptr))
            {
                return false;
            }

            data += bytesWritten;
            remaining -= bytesWritten;
        }

        return FlushFileBuffers(m_handle) != FALSE;
    }

    [[nodiscard]]
    bool readLine(std::string *line, std::chrono::milliseconds timeout)
    {
        if (line == nullptr)
        {
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true)
        {
            const std::size_t newlinePosition = m_readBuffer.find('\n');
            if (newlinePosition != std::string::npos)
            {
                *line = m_readBuffer.substr(0, newlinePosition);
                m_readBuffer.erase(0, newlinePosition + 1U);
                if (!line->empty() && line->back() == '\r')
                {
                    line->pop_back();
                }
                return true;
            }

            DWORD availableBytes = 0;
            if (!PeekNamedPipe(m_handle, nullptr, 0, nullptr, &availableBytes, nullptr))
            {
                return false;
            }

            if (availableBytes == 0)
            {
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            const DWORD chunkSize = availableBytes > 4096U ? 4096U : availableBytes;
            char buffer[4096];
            DWORD bytesRead = 0;
            if (!ReadFile(m_handle, buffer, chunkSize, &bytesRead, nullptr) || bytesRead == 0)
            {
                return false;
            }

            m_readBuffer.append(buffer, static_cast<std::size_t>(bytesRead));
        }
    }

private:
    HANDLE m_handle {INVALID_HANDLE_VALUE};
    std::string m_readBuffer;
};

#else

class NativeConnection final
{
public:
    NativeConnection() = default;

    ~NativeConnection()
    {
        close();
    }

    [[nodiscard]]
    bool isOpen() const
    {
        return m_fd >= 0;
    }

    void close()
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
            m_fd = -1;
        }

        m_readBuffer.clear();
    }

    [[nodiscard]]
    bool connect(const std::string &endpoint, std::chrono::milliseconds)
    {
        close();

        m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_fd < 0)
        {
            return false;
        }

#ifdef SO_NOSIGPIPE
        const int disableSigPipe = 1;
        ::setsockopt(m_fd, SOL_SOCKET, SO_NOSIGPIPE, &disableSigPipe, sizeof(disableSigPipe));
#endif

        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        if (endpoint.size() >= sizeof(address.sun_path))
        {
            close();
            return false;
        }

        std::memcpy(address.sun_path, endpoint.c_str(), endpoint.size() + 1U);
        if (::connect(m_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
        {
            close();
            return false;
        }

        return true;
    }

    [[nodiscard]]
    bool writeAll(const std::string &payload)
    {
        const char *data = payload.data();
        std::size_t remaining = payload.size();

        while (remaining > 0)
        {
#ifdef MSG_NOSIGNAL
            constexpr int sendFlags = MSG_NOSIGNAL;
#else
            constexpr int sendFlags = 0;
#endif
            const ssize_t bytesWritten = ::send(m_fd, data, remaining, sendFlags);
            if (bytesWritten < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }

            data += bytesWritten;
            remaining -= static_cast<std::size_t>(bytesWritten);
        }

        return true;
    }

    [[nodiscard]]
    bool readLine(std::string *line, std::chrono::milliseconds timeout)
    {
        if (line == nullptr)
        {
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true)
        {
            const std::size_t newlinePosition = m_readBuffer.find('\n');
            if (newlinePosition != std::string::npos)
            {
                *line = m_readBuffer.substr(0, newlinePosition);
                m_readBuffer.erase(0, newlinePosition + 1U);
                if (!line->empty() && line->back() == '\r')
                {
                    line->pop_back();
                }
                return true;
            }

            auto remainingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            if (remainingDuration.count() <= 0)
            {
                return false;
            }

            pollfd descriptor {};
            descriptor.fd = m_fd;
            descriptor.events = POLLIN;
            const int pollResult = ::poll(&descriptor, 1, static_cast<int>(remainingDuration.count()));
            if (pollResult < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }
            if (pollResult == 0)
            {
                return false;
            }
            if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            {
                return false;
            }

            char buffer[4096];
            const ssize_t bytesRead = ::recv(m_fd, buffer, sizeof(buffer), 0);
            if (bytesRead < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }
            if (bytesRead == 0)
            {
                return false;
            }

            m_readBuffer.append(buffer, static_cast<std::size_t>(bytesRead));
        }
    }

private:
    int m_fd {-1};
    std::string m_readBuffer;
};

#endif

} // namespace Detail

class PolyShowIpcClient final
{
public:
    explicit PolyShowIpcClient(ClientConfig config = {})
        : m_config(std::move(config))
    {
        if (m_config.endpoint.empty())
        {
            m_config.endpoint = nativeEndpoint();
        }
        if (m_config.queueCapacity == 0)
        {
            m_config.queueCapacity = 1;
        }

        m_workerThread = std::thread([this]() {
            workerLoop();
        });
    }

    ~PolyShowIpcClient()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_running = false;
            m_statsDropped.fetch_add(static_cast<std::uint64_t>(m_queue.size()), std::memory_order_relaxed);
            m_queue.clear();
        }

        m_queueNotEmpty.notify_all();
        m_queueNotFull.notify_all();
        m_queueStateChanged.notify_all();

        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    }

    PolyShowIpcClient(const PolyShowIpcClient &) = delete;
    PolyShowIpcClient &operator=(const PolyShowIpcClient &) = delete;

    [[nodiscard]]
    bool enqueue(const PrimitiveMessage &message)
    {
        if (!Detail::validatePrimitiveMessage(message))
        {
            return false;
        }

        PrimitiveMessage queuedMessage = message;
        if (queuedMessage.requestId.empty())
        {
            queuedMessage.requestId = std::string("polyshow-ipc-")
                + std::to_string(m_nextRequestId.fetch_add(1, std::memory_order_relaxed));
        }

        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running)
        {
            return false;
        }

        if (m_config.queueMode == QueueMode::BoundedBlocking)
        {
            m_queueNotFull.wait(lock, [this]() {
                return !m_running || m_queue.size() < m_config.queueCapacity;
            });
            if (!m_running)
            {
                return false;
            }
        }

        m_queue.push_back(std::move(queuedMessage));
        m_statsEnqueued.fetch_add(1, std::memory_order_relaxed);
        lock.unlock();
        m_queueNotEmpty.notify_one();
        m_queueStateChanged.notify_all();
        return true;
    }

    [[nodiscard]]
    bool flush(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queueStateChanged.wait_for(lock, timeout, [this]() {
            return m_queue.empty() && !m_hasInflightMessage;
        });
    }

    [[nodiscard]]
    ClientStats stats() const
    {
        ClientStats snapshot;
        snapshot.enqueued = m_statsEnqueued.load(std::memory_order_relaxed);
        snapshot.sent = m_statsSent.load(std::memory_order_relaxed);
        snapshot.dropped = m_statsDropped.load(std::memory_order_relaxed);
        snapshot.connectFailures = m_statsConnectFailures.load(std::memory_order_relaxed);
        snapshot.writeFailures = m_statsWriteFailures.load(std::memory_order_relaxed);
        snapshot.responseFailures = m_statsResponseFailures.load(std::memory_order_relaxed);
        return snapshot;
    }

private:
    void workerLoop()
    {
        Detail::NativeConnection connection;

        while (true)
        {
            PrimitiveMessage message;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_queueNotEmpty.wait(lock, [this]() {
                    return !m_running || !m_queue.empty();
                });

                if (!m_running && m_queue.empty())
                {
                    break;
                }

                message = std::move(m_queue.front());
                m_queue.pop_front();
                m_hasInflightMessage = true;
                m_queueNotFull.notify_one();
                m_queueStateChanged.notify_all();
            }

            processMessage(connection, message);

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_hasInflightMessage = false;
            }
            m_queueStateChanged.notify_all();
        }

        connection.close();
    }

    void processMessage(Detail::NativeConnection &connection, const PrimitiveMessage &message)
    {
        if (!connection.isOpen() && !connection.connect(m_config.endpoint, Detail::kConnectTimeout))
        {
            m_statsConnectFailures.fetch_add(1, std::memory_order_relaxed);
            m_statsDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        const std::string payload = Detail::serializeMessageJson(message);
        if (!connection.writeAll(payload))
        {
            m_statsWriteFailures.fetch_add(1, std::memory_order_relaxed);
            m_statsDropped.fetch_add(1, std::memory_order_relaxed);
            connection.close();
            return;
        }

        std::string responseLine;
        if (!connection.readLine(&responseLine, Detail::kResponseTimeout))
        {
            m_statsResponseFailures.fetch_add(1, std::memory_order_relaxed);
            m_statsDropped.fetch_add(1, std::memory_order_relaxed);
            connection.close();
            return;
        }

        bool ok = false;
        if (!Detail::parseResponseOkField(responseLine, &ok))
        {
            m_statsResponseFailures.fetch_add(1, std::memory_order_relaxed);
            m_statsDropped.fetch_add(1, std::memory_order_relaxed);
            connection.close();
            return;
        }

        if (!ok)
        {
            m_statsResponseFailures.fetch_add(1, std::memory_order_relaxed);
            m_statsDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        m_statsSent.fetch_add(1, std::memory_order_relaxed);
    }

    ClientConfig m_config;
    std::thread m_workerThread;
    mutable std::mutex m_mutex;
    std::condition_variable m_queueNotEmpty;
    std::condition_variable m_queueNotFull;
    std::condition_variable m_queueStateChanged;
    std::deque<PrimitiveMessage> m_queue;
    bool m_running {true};
    bool m_hasInflightMessage {false};
    std::atomic<std::uint64_t> m_nextRequestId {1};
    std::atomic<std::uint64_t> m_statsEnqueued {0};
    std::atomic<std::uint64_t> m_statsSent {0};
    std::atomic<std::uint64_t> m_statsDropped {0};
    std::atomic<std::uint64_t> m_statsConnectFailures {0};
    std::atomic<std::uint64_t> m_statsWriteFailures {0};
    std::atomic<std::uint64_t> m_statsResponseFailures {0};
};

} // namespace PolyShow::Ipc
