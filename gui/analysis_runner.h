#pragma once

#include "analysis.h"

#include <atomic>
#include <functional>
#include <thread>

class QObject;

class AnalysisRunner {
public:
    using FinishHandler = std::function<void(const crc::CodeAnalysis&)>;

    explicit AnalysisRunner(QObject* receiver);
    ~AnalysisRunner();

    AnalysisRunner(const AnalysisRunner&) = delete;
    AnalysisRunner& operator=(const AnalysisRunner&) = delete;

    void start(const crc::CyclicCode& code, FinishHandler onFinished);
    void cancel();
    bool running() const;

private:
    void work(const crc::CyclicCode& code, const FinishHandler& onFinished);
    void deliver(std::function<void()> action);
    void join();

    QObject* receiver_;
    std::thread thread_;
    std::atomic<bool> cancelRequested_{false};
    bool running_ = false;
};
