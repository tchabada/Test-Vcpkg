#include "Application.hpp"

#include <signal.h>

#include <iostream>

#include <stdlib.h>
#include <time.h>

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
PeriodicTask::PeriodicTask(boost::asio::io_context& ioContext, size_t period, Task task)
    : mIOContext{ioContext}
    , mTimer{mIOContext}
    , mTask{std::move(task)}
    , mPeriod{period}
{
}
//--------------------------------------------------------------------------------
void PeriodicTask::start()
{
    mIOContext.post([this, self = shared_from_this()]() { execute({}); });
}
//--------------------------------------------------------------------------------
void PeriodicTask::stop()
{
    mIOContext.post([this, self = shared_from_this()]()
                    { mTimer.expires_at(boost::asio::steady_timer::clock_type::time_point::min()); });
}
//--------------------------------------------------------------------------------
void PeriodicTask::execute(boost::system::error_code const& errorCode)
{
    if (!errorCode && mTimer.expires_at() != boost::asio::steady_timer::clock_type::time_point::min())
    {
        mTask();
        mTimer.expires_from_now(std::chrono::milliseconds(mPeriod));
        mTimer.async_wait(
            [this, self = shared_from_this()](boost::system::error_code const& errorCode) { execute(errorCode); });
    }
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
Application::Application()
    : mSignalSet{mIOContext}
    , mException{nullptr}
{
}
//--------------------------------------------------------------------------------
void Application::run(int argc, char* argv[])
{
    try
    {
        std::cout << "Start"
                  << "\n";

        mSignalSet.add(SIGINT);
        mSignalSet.add(SIGTERM);

        mSignalSet.async_wait([this](const boost::system::error_code&, int) {
            std::cout << "Signal"
                      << "\n";
            stopWork();
        });

        bool check = false;
        auto checkTask = createPeriodicTask(updatablesPeriod_ms, [this, &check]() {
            static size_t count = 0;

            std::cout << "checking...\n";

            if (count++ > 1)
            {
                check = true;
                mIOContext.stop();
            }
        });

        mIOContext.run();

        if (!check)
        {
            throw std::logic_error("initialization error");
        }

        checkTask->stop();
        mIOContext.restart();

        mUpdatables.push_back(std::make_shared<Updatable>(*this, "Updatable 1"));
        mUpdatables.push_back(std::make_shared<Updatable>(*this, "Updatable 2"));

        createPeriodicTask(updatablesPeriod_ms, [this]() {
            for (auto&& u : mUpdatables)
            {
                u->update();
            }
        });

        mServices.push_back(std::make_shared<Service>(*this, "Service 1"));
        mServices.push_back(std::make_shared<Service>(*this, "Service 2"));

        for (auto&& s : mServices)
        {
            s->init();
        }

        for (size_t i = 0; i < numThreads; ++i)
        {
            mThreads[i] = std::thread{[this]() {
                try
                {
                    mIOContext.run();
                }
                catch (const std::exception&)
                {
                    mException = std::current_exception();
                    stopWork();
                }
            }};
        }

        mIOContext.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';

        mException = std::current_exception();
        stopWork();
    }

    for (auto&& t : mThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    if (!mException)
    {
        for (auto&& u : mUpdatables)
        {
            u->store();
        }
    }
}
//--------------------------------------------------------------------------------
void Application::stop() { stopWork(); }
//--------------------------------------------------------------------------------
std::shared_ptr<PeriodicTask> Application::createPeriodicTask(size_t period, PeriodicTask::Task task)
{
    auto periodicTask = PeriodicTask::create(mIOContext, period, task);

    periodicTask->start();

    return periodicTask;
}
//--------------------------------------------------------------------------------
void Application::stopWork()
{
    std::cout << "Stop"
              << "\n";

    mIOContext.stop();
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void Updatable::update()
{
    std::cout << mName << "\n";

    mApplication.post(
        &Updatable::compute,
        [](const boost::system::error_code& errorCode, auto&& result) {
            if (!errorCode)
            {
                std::cout << result << "\n";
            }
        },
        this, 21);
}
//--------------------------------------------------------------------------------
void Updatable::store()
{
    std::cout << "storing"
              << "\n";
}
//--------------------------------------------------------------------------------
int Updatable::compute(int x) { return x + x; }
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void Service::init()
{
    auto l = [this]() { update(); };

    mPeriodicTask = mApplication.createPeriodicTask(servicesPeriod_ms, l);
}
//--------------------------------------------------------------------------------
void Service::update()
{
    std::cout << mName << "\n";

    mApplication.post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); },
                      [](const boost::system::error_code& errorCode) {
                          if (!errorCode)
                          {
                              std::cout << "computation done"
                                        << "\n";
                          }
                      });
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
