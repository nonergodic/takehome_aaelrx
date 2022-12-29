#ifndef SOURCE_SERVICE_HPP
#define SOURCE_SERVICE_HPP

#include <functional>

#include "common.hpp"
#include "microservice.hpp"
#include "record_types.hpp"

class SourceService : public Microservice {
  public:
    //maximum entropy of a random message (about 100MB - sqlite has an upper limit of ~1GB)
    auto static constexpr max_random_message_bytes = 1e8;

    using RecordCallback = std::function<void (std::stop_token, Record)>;

    SourceService(std::string const & dbfile);

    bool is_empty() const;
    void populate(size_t count);

    void start(RecordCallback && cb, size_t log_frequency);
  
  private:
    void work_loop(std::stop_token stop, RecordCallback && cb, size_t log_frequency);

    std::string dbfile_;
};

#endif
