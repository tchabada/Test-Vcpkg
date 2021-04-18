#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <memory>
#include <type_traits>

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
constexpr size_t numThreads = 2;
constexpr size_t updatablesPeriod_ms = 1000;
constexpr size_t servicesPeriod_ms = 2000;
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
class PeriodicTask : public std::enable_shared_from_this<PeriodicTask>
{
public:
    using Task = std::function<void()>;

    template <typename... Args> static auto create(Args&&... args)
    {
        return std::shared_ptr<PeriodicTask>(new PeriodicTask(std::forward<decltype(args)>(args)...));
    }

    void start();
    void stop();

    PeriodicTask(const PeriodicTask&) = delete;
    PeriodicTask& operator=(const PeriodicTask&) = delete;

private:
    PeriodicTask(boost::asio::io_context& ioContext, size_t period, Task task);

    void execute(boost::system::error_code const& errorCode);

private:
    boost::asio::io_context& mIOContext;
    boost::asio::steady_timer mTimer;
    Task mTask;
    size_t mPeriod;
};
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
class Updatable;
class Service;

class Application
{
public:
    Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run(int argc, char* argv[]);
    void stop();

    std::shared_ptr<PeriodicTask> createPeriodicTask(size_t period, PeriodicTask::Task task);

    template <typename F> void post(F&& f) { boost::asio::post(mIOContext, std::forward<F>(f)); }

    template <typename F, typename Token, typename... Args> auto post(F&& f, Token&& token, Args&&... args)
    {
        if constexpr (std::is_same_v<std::invoke_result_t<F, Args...>, void>)
        {
            using result_type =
                typename boost::asio::async_result<std::decay_t<Token>, void(boost::system::error_code)>;
            typename result_type::completion_handler_type handler(std::forward<Token>(token));
            result_type result(handler);
            boost::asio::post(mIOContext,
                              [handler, f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
                                  std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                                  handler(boost::system::error_code{});
                              });
            return result.get();
        }
        else
        {
            using result_type =
                typename boost::asio::async_result<std::decay_t<Token>,
                                                   void(boost::system::error_code, std::invoke_result_t<F, Args...>)>;
            typename result_type::completion_handler_type handler(std::forward<Token>(token));
            result_type result(handler);
            boost::asio::post(
                mIOContext, [handler, f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
                    handler(boost::system::error_code{}, std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
                });
            return result.get();
        }
    }

private:
    void stopWork();

private:
    boost::asio::io_context mIOContext;
    boost::asio::signal_set mSignalSet;
    std::array<std::thread, numThreads> mThreads;
    std::exception_ptr mException;

    std::vector<std::shared_ptr<Updatable>> mUpdatables;
    std::vector<std::shared_ptr<Service>> mServices;
};
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
class Updatable
{
public:
    explicit Updatable(Application& application, std::string name)
        : mApplication{application}
        , mName{std::move(name)}
    {
    }

    void update();
    void store();
    int compute(int in);

private:
    Application& mApplication;
    std::string mName;
};
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
class Service
{
public:
    explicit Service(Application& application, std::string name)
        : mApplication{application}
        , mName{std::move(name)}
    {
    }

    void init();
    void update();

private:
    Application& mApplication;
    std::shared_ptr<PeriodicTask> mPeriodicTask;
    std::string mName;
};
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
