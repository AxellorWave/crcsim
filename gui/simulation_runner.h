#pragma once

#include "simulator.h"

#include <QString>

#include <atomic>
#include <functional>
#include <thread>

class QObject;

class SimulationRunner {
public:
    using ProgressHandler = std::function<void(const crc::Counters&)>;
    using FinishHandler = std::function<void(const crc::SimulationResult&)>;
    using FailureHandler = std::function<void(const QString&)>;

    explicit SimulationRunner(QObject* receiver);
    ~SimulationRunner();

    SimulationRunner(const SimulationRunner&) = delete;
    SimulationRunner& operator=(const SimulationRunner&) = delete;

    void start(const crc::SimulationParams& params, ProgressHandler onProgress, FinishHandler onFinished,
        FailureHandler onFailed);
    void cancel();
    bool running() const;

private:
    void work(const crc::SimulationParams& params, const ProgressHandler& onProgress, const FinishHandler& onFinished,
        const FailureHandler& onFailed);
    void deliver(std::function<void()> action);
    void join();

    QObject* receiver_;
    std::thread thread_;
    std::atomic<bool> cancelRequested_{false};
    bool running_ = false;
};
