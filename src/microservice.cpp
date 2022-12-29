#include "microservice.hpp"

void Microservice::log(std::string && logline) {
  log_signal_(std::make_shared<std::string>(std::move(logline)));
}

void Microservice::request_stop() {
  if (!thread_.has_value())
    throw make_exception<Exception>("not started");
  
  thread_->request_stop();
}

void Microservice::blocking_stop() {
  if (!thread_.has_value())
    throw make_exception<Exception>("not started");
  
  thread_.reset(); //will automatically invoke stop token and join thread
}

void Microservice::join() {
  if (!thread_.has_value())
    throw make_exception<Exception>("not started");
  
  thread_->join();
}

// void Microservice::start_thread(std::function<void (std::stop_token)> && work_loop) {
//   if (thread_.has_value())
//     throw make_exception<Exception>("already started");
  
//   thread_ = std::jthread(work_loop);
// }