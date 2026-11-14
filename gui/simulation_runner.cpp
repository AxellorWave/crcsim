#include "simulation_runner.h"

#include <QMetaObject>
#include <QObject>

#include <exception>

namespace {

const std::uint64_t progressInterval = 16384;

}

SimulationRunner::SimulationRunner(QObject* receiver)
    : receiver_(receiver)
{
}

SimulationRunner::~SimulationRunner()
{
    cancel();
    join();
}

void SimulationRunner::start(
    const crc::SimulationParams& params, ProgressHandler onProgress, FinishHandler onFinished, FailureHandler onFailed)
{
    if (running_)
        return;
    join();
    cancelRequested_ = false;
    running_ = true;
    thread_ = std::thread([this, params, onProgress, onFinished, onFailed] {
        work(params, onProgress, onFinished, onFailed);
    });
}

void SimulationRunner::cancel()
{
    cancelRequested_ = true;
}

bool SimulationRunner::running() const
{
    return running_;
}

void SimulationRunner::work(const crc::SimulationParams& params, const ProgressHandler& onProgress,
    const FinishHandler& onFinished, const FailureHandler& onFailed)
{
    crc::RunControl control;
    control.cancelRequested = &cancelRequested_;
    control.progressInterval = progressInterval;
    control.onProgress = [this, onProgress](const crc::Counters& counters) {
        deliver([onProgress, counters] { onProgress(counters); });
    };
    try {
        crc::Simulator simulator(params);
        const crc::SimulationResult result = simulator.run(control);
        deliver([this, onFinished, result] {
            join();
            onFinished(result);
        });
    } catch (const std::exception& error) {
        const QString message = QString::fromUtf8(error.what());
        deliver([this, onFailed, message] {
            join();
            onFailed(message);
        });
    }
}

void SimulationRunner::deliver(std::function<void()> action)
{
    QMetaObject::invokeMethod(receiver_, std::move(action), Qt::QueuedConnection);
}

void SimulationRunner::join()
{
    if (thread_.joinable())
        thread_.join();
    running_ = false;
}
