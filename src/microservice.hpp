#ifndef MICROSERVICE_HPP
#define MICROSERVICE_HPP

#include <memory>
#include <optional>
#include <thread>

#include <boost/signals2.hpp>

#include "common.hpp"

class Microservice {
  public:
    using LogLinePtr = std::shared_ptr<std::string>;

    virtual ~Microservice() {}

    template <typename FuncT>
    auto subscribe_logs(FuncT && cb) {return log_signal_.connect(cb);}

    void request_stop();
    void blocking_stop();
    void join();
    
  protected:
    void log(std::string && logline);

    template<typename FuncT, typename... ArgsT>
    void start_thread(FuncT && work_loop, ArgsT &&... args);
  
  private:
    using LogSignal = boost::signals2::signal<void (LogLinePtr const & logline)>;

    std::optional<std::jthread> thread_;
    LogSignal log_signal_;
};

template<typename FuncT, typename... ArgsT>
void Microservice::start_thread(FuncT && work_loop, ArgsT &&... args) {
  if (thread_.has_value())
    throw make_exception<Exception>("already started");
  
  //TODO wrap work_loop in try-catch-all
  thread_ = std::jthread(work_loop, args...);
}

#endif
