#ifndef BATCH_SERVICE_HPP
#define BATCH_SERVICE_HPP

#include "common.hpp"
#include "microservice.hpp"
#include "threadsafe_queue.hpp"
#include "record_types.hpp"

class KeyService;

class BatchService : public Microservice {
  public:
    using SignedBatchCallback = std::function<void (std::stop_token, SignedBatch)>;

    BatchService(size_t batch_size, size_t signing_threads);

    void put(std::stop_token stop, Record && record);

    void start(SignedBatchCallback && cb, KeyService & key_service, size_t log_frequency);

  private:
    void work_loop(
      std::stop_token stop,
      SignedBatchCallback && cb,
      KeyService & key_service,
      size_t log_frequency
    );
    
    size_t batch_size_;
    size_t signing_threads_;
    ThreadsafeQueue<Record, WaitUntilCapacityAvailable> record_queue_;
};

#endif
