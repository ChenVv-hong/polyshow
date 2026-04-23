#include "PolyShowIpcClient.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{

using PolyShow::Ipc::ClientConfig;
using PolyShow::Ipc::PolyShowIpcClient;
using PolyShow::Ipc::PrimitiveKind;
using PolyShow::Ipc::PrimitiveMessage;
using PolyShow::Ipc::PrimitiveStyle;
using PolyShow::Ipc::QueueMode;

constexpr const char *kTargetLayerName = "IPC Demo";
constexpr std::size_t kWorkerThreadCount = 4;
constexpr std::size_t kRepeatPerWorker = 3;

PrimitiveMessage makePointMessage(const std::string &name, double x, double y)
{
    PrimitiveStyle style;
    style.color = "#D94841";
    style.pointSize = 5.0;

    PrimitiveMessage message;
    message.layerName = kTargetLayerName;
    message.kind = PrimitiveKind::Point;
    message.name = name;
    message.points = {{x, y}};
    message.style = style;
    return message;
}

PrimitiveMessage makePolylineMessage(const std::string &name, double offsetX)
{
    PrimitiveStyle style;
    style.color = "#2259B4";
    style.width = 2.0;

    PrimitiveMessage message;
    message.layerName = kTargetLayerName;
    message.kind = PrimitiveKind::Polyline;
    message.name = name;
    message.points = {
        {offsetX + 0.0, 0.0},
        {offsetX + 12.0, 6.0},
        {offsetX + 24.0, 3.0}
    };
    message.style = style;
    return message;
}

PrimitiveMessage makePolygonMessage(const std::string &name, double offsetX)
{
    PrimitiveStyle style;
    style.color = "#1F7A4D";
    style.fillColor = "#1F7A4D55";
    style.fillEnabled = true;
    style.width = 1.5;

    PrimitiveMessage message;
    message.layerName = kTargetLayerName;
    message.kind = PrimitiveKind::Polygon;
    message.name = name;
    message.points = {
        {offsetX + 0.0, 0.0},
        {offsetX + 10.0, 18.0},
        {offsetX + 22.0, 4.0}
    };
    message.style = style;
    return message;
}

} // namespace

int main()
{
    std::cout << "PolyShow IPC sender demo\n";
    std::cout << "Target IPC layer: " << kTargetLayerName << "\n";
    std::cout << "Endpoint: " << PolyShow::Ipc::nativeEndpoint() << "\n";
    std::cout << "This demo will enqueue point/polyline/polygon messages from multiple threads.\n";

    ClientConfig config;
    config.queueMode = QueueMode::BoundedBlocking;
    config.queueCapacity = 4096;

    PolyShowIpcClient client(config);
    std::vector<std::thread> workers;
    workers.reserve(kWorkerThreadCount);

    for (std::size_t workerIndex = 0; workerIndex < kWorkerThreadCount; ++workerIndex)
    {
        workers.emplace_back([workerIndex, &client]() {
            for (std::size_t repeatIndex = 0; repeatIndex < kRepeatPerWorker; ++repeatIndex)
            {
                const std::string suffix = std::to_string(workerIndex) + "-" + std::to_string(repeatIndex);

                const bool pointAccepted =
                    client.enqueue(makePointMessage("demo-point-" + suffix, 5.0 + workerIndex * 10.0, repeatIndex * 6.0));
                const bool polylineAccepted =
                    client.enqueue(makePolylineMessage("demo-line-" + suffix, workerIndex * 30.0));
                const bool polygonAccepted =
                    client.enqueue(makePolygonMessage("demo-polygon-" + suffix, workerIndex * 30.0));

                if (!pointAccepted || !polylineAccepted || !polygonAccepted)
                {
                    std::cerr << "enqueue rejected for batch " << suffix << "\n";
                }
            }
        });
    }

    for (std::thread &worker : workers)
    {
        worker.join();
    }

    const bool flushCompleted = client.flush(std::chrono::seconds(10));
    const PolyShow::Ipc::ClientStats stats = client.stats();

    std::cout << "flushCompleted=" << (flushCompleted ? "true" : "false") << "\n";
    std::cout << "enqueued=" << stats.enqueued << "\n";
    std::cout << "sent=" << stats.sent << "\n";
    std::cout << "dropped=" << stats.dropped << "\n";
    std::cout << "connectFailures=" << stats.connectFailures << "\n";
    std::cout << "writeFailures=" << stats.writeFailures << "\n";
    std::cout << "responseFailures=" << stats.responseFailures << "\n";

    if (stats.sent == 0)
    {
        std::cout << "If you expected geometry to appear, start PolyShow's IPC listener and create an IPC layer named \""
                  << kTargetLayerName << "\" first.\n";
    }

    return flushCompleted ? 0 : 2;
}
