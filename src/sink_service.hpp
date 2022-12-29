#ifndef SINK_SERVICE_HPP
#define SINK_SERVICE_HPP

#include "common.hpp"
#include "microservice.hpp"
#include "threadsafe_queue.hpp"
#include "record_types.hpp"

class SinkService : public Microservice {
  public:
    SinkService(std::string const & dbfile, size_t queue_capacity);

    void put(std::stop_token stop, SignedBatch && signed_batch);

    void start(size_t log_frequency);
  
  private:
    void work_loop(std::stop_token stop, size_t log_frequency);

    std::string dbfile_;
    ThreadsafeQueue<SignedBatch, WaitUntilCapacityAvailable> batch_queue_;
};

#endif
