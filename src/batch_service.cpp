#include "batch_service.hpp"

#include "key.hpp"
#include "key_service.hpp"

BatchService::BatchService(
  size_t batch_size,
  size_t signing_threads
) : batch_size_(batch_size),
    signing_threads_(signing_threads),
    record_queue_(batch_size * signing_threads) {
}

void BatchService::put(std::stop_token stop, Record && record) {
  record_queue_.push(stop, std::move(record));
}

void BatchService::start(
  SignedBatchCallback && cb,
  KeyService & key_service,
  size_t log_frequency
) {
  using namespace std::placeholders;
  start_thread(
    std::bind(&BatchService::work_loop, this, _1, _2, _3, _4),
    std::move(cb),
    std::ref(key_service),
    log_frequency
  );
}

void BatchService::work_loop(
  std::stop_token stop,
  SignedBatchCallback && cb,
  KeyService & key_service,
  size_t log_frequency
) {
  //using namespace std::chrono_literals;
  //std::this_thread::sleep_for(1s);
  log("BatchService: work_loop started");
  
  try {
    for (size_t i = 1; !stop.stop_requested(); ++i) {
      //fill batch
      auto batch = std::vector<Record>();
      batch.reserve(batch_size_);
      for (size_t j = 0; j < batch_size_; ++j) {
        auto record = record_queue_.pop(stop);
        batch.emplace_back(std::move(record));
      }
      
      //acquire key
      auto key = key_service.acquire_key();
      
      //sign batch (move to separate thread)
      auto signed_batch = SignedBatch();
      signed_batch.reserve(batch_size_);
      for (auto const & record : batch) {
        if (stop.stop_requested())
          break;
        signed_batch.emplace_back(record.id, key.get_public_key(), key.sign(record.message));
      }

      if (stop.stop_requested())
          break;

      //invoke callback
      cb(stop, std::move(signed_batch));

      if (i % log_frequency == 0)
        log("BatchService: signed " + std::to_string(i) + " batches");
    }
  }
  catch (StopRequested const &) {}
  log("BatchService: work_loop ended");
}