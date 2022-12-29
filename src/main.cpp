#include <iostream>
#include <thread>
#include <chrono>

#include "record_types.hpp"
#include "threadsafe_queue.hpp"
#include "source_service.hpp"
#include "key_service.hpp"
#include "batch_service.hpp"
#include "sink_service.hpp"

//int main(int argc, char** argv) {
int main() {
  try {
    auto constexpr message_count = 1000;
    auto constexpr key_count = 10;
    auto constexpr batch_size = 100;
    auto constexpr signing_threads = 1; //only supports one thread atm
    auto constexpr batch_log_frequency = 1;
    auto constexpr sink_queue_capacity = 10;

    auto log_queue =
      ThreadsafeQueue<Microservice::LogLinePtr, WaitUntilCapacityAvailable>(1000);
    auto push_log = [&log_queue](auto log_line) {
      log_queue.push(Microservice::LogLinePtr{log_line}); //this is terrible
    };

    auto services = std::vector<std::unique_ptr<Microservice>>();

    services.emplace_back(std::make_unique<SourceService>("messages.db"));
    auto source_service = dynamic_cast<SourceService*>(services.back().get());

    if (source_service->is_empty()) {
      source_service->populate(message_count);
      return EXIT_SUCCESS;
    }

    services.emplace_back(std::make_unique<KeyService>(key_count));
    auto key_service = dynamic_cast<KeyService*>(services.back().get());

    services.emplace_back(std::make_unique<BatchService>(batch_size, signing_threads));
    auto batch_service = dynamic_cast<BatchService*>(services.back().get());

    services.emplace_back(std::make_unique<SinkService>("signed.db", sink_queue_capacity));
    auto sink_service = dynamic_cast<SinkService*>(services.back().get());

    auto log_thread = std::jthread([&log_queue](std::stop_token stop) {
      try {
        while (!stop.stop_requested()) {
          auto log_line = log_queue.pop(stop);
          std::cout << *log_line << std::endl;
        }
      }
      catch (StopRequested const &) {}
    });

    for (auto & service : services)
      service->subscribe_logs(push_log);

    sink_service->start(batch_log_frequency);

    batch_service->start(
      [sink_service](std::stop_token stop, SignedBatch && signed_batch) {
        sink_service->put(stop, std::move(signed_batch));
      },
      *key_service,
      batch_log_frequency
    );

    source_service->start(
      [batch_service](std::stop_token stop, Record && record) {
        batch_service->put(stop, std::move(record));
      },
      batch_size
    );

    source_service->join();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s); //even more terrible

    for (auto & service : services)
      service->request_stop();
    
    services.clear();
  }
  catch (std::exception const & e) {
    std::cout << "caught fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}