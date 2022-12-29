#include "sink_service.hpp"

#include <random>
#include <numeric>

#include <SQLiteCpp/SQLiteCpp.h>

#include "crypto_sizes.hpp"

auto constexpr hex_digits_per_byte = size_t{2};

SinkService::SinkService(
  std::string const & dbfile,
  size_t queue_capacity
) : dbfile_(dbfile), 
    batch_queue_(queue_capacity) {
  auto db = SQLite::Database(dbfile_, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
  db.exec("DROP TABLE IF EXISTS signed");
  db.exec(
  "CREATE TABLE signed ("
    "id INTEGER PRIMARY KEY, "
    "signature CHAR(" + std::to_string(hex_digits_per_byte * signature_bytes) + "), "
    "signer CHAR(" + std::to_string(hex_digits_per_byte * public_key_bytes) + ")"
  ")"
);
}

void SinkService::put(std::stop_token stop, SignedBatch && signed_batch) {
  batch_queue_.push(stop, std::move(signed_batch));
}

void SinkService::start(size_t log_frequency) {
  using namespace std::placeholders;
  start_thread(std::bind(&SinkService::work_loop, this, _1, _2), log_frequency);
}

void SinkService::work_loop(std::stop_token stop, size_t log_frequency) {
  log("SinkService: work_loop started");
  auto db = SQLite::Database(dbfile_, SQLite::OPEN_READWRITE);
  auto insert_query = SQLite::Statement(db, "INSERT INTO signed VALUES (?, ?, ?)");
  try {
    for (size_t i = 1; true; ++i) {
        auto batch = batch_queue_.pop(stop);
        auto transaction = SQLite::Transaction(db);
        for (auto const & signed_record : batch) {
          insert_query.bind(1, signed_record.id);
          insert_query.bindNoCopy(2, signed_record.signature);
          insert_query.bindNoCopy(3, signed_record.signer);
          if (insert_query.exec() != 1)
            throw make_exception<Exception>(
              "insert failed for id: " +
              std::to_string(signed_record.id)
            );
          insert_query.reset();
        }
        transaction.commit();
      
      if (i % log_frequency == 0)
        log("SinkService: wrote " + std::to_string(i) + " batches");
    }
  }
  catch (StopRequested const &) {}
  log("SinkService: work_loop ended");
}