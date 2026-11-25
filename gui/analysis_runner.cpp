#include "analysis_runner.h"

#include <QMetaObject>
#include <QObject>

#include <exception>

AnalysisRunner::AnalysisRunner(QObject* receiver)
    : receiver_(receiver)
{
}

AnalysisRunner::~AnalysisRunner()
{
    cancel();
    join();
}

void AnalysisRunner::start(const crc::CyclicCode& code, FinishHandler onFinished)
{
    if (running_)
        return;
    join();
    cancelRequested_ = false;
    running_ = true;
    thread_ = std::thread([this, code, onFinished] { work(code, onFinished); });
}

void AnalysisRunner::cancel()
{
    cancelRequested_ = true;
}

bool AnalysisRunner::running() const
{
    return running_;
}

void AnalysisRunner::work(const crc::CyclicCode& code, const FinishHandler& onFinished)
{
    crc::CodeAnalysis analysis;
    try {
        analysis = crc::analyzeCode(code, &cancelRequested_);
    } catch (const std::exception& error) {
        analysis.code = code;
        analysis.failureReason = error.what();
    }
    deliver([this, onFinished, analysis] {
        join();
        onFinished(analysis);
    });
}

void AnalysisRunner::deliver(std::function<void()> action)
{
    QMetaObject::invokeMethod(receiver_, std::move(action), Qt::QueuedConnection);
}

void AnalysisRunner::join()
{
    if (thread_.joinable())
        thread_.join();
    running_ = false;
}
