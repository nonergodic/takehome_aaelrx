#include "source_service.hpp"

#include <random>
#include <numeric>

#include <SQLiteCpp/SQLiteCpp.h>

auto constexpr hex_digits_per_byte = size_t{2};

SourceService::SourceService(std::string const & dbfile) : 
  dbfile_(dbfile) {
    auto db = SQLite::Database(dbfile_, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
    auto table_exists = [&]() {
      return SQLite::Statement(
        db,
        "SELECT name FROM sqlite_master WHERE type='table' AND name='messages'"
      ).executeStep();
    };

    if (!table_exists())
      db.exec("CREATE TABLE messages (id INTEGER PRIMARY KEY, size INTERGER, message TEXT)");
}

bool SourceService::is_empty() const {
  auto db = SQLite::Database(dbfile_, SQLite::OPEN_READONLY);
  auto query = SQLite::Statement(db, "SELECT COUNT(*) FROM messages");
  query.executeStep();
  auto message_count = query.getColumn(0).getInt();
  return message_count == 0;
}

void SourceService::populate(size_t count) {
  auto rng = std::mt19937_64(std::random_device()());
  auto pareto_distribution = [&rng](auto shape) {
    return [&rng, shape]() {
      //see https://en.wikipedia.org/wiki/Pareto_distribution#Relation_to_the_exponential_distribution
      auto static exp_distr = std::exponential_distribution<>(shape);
      auto res = std::exp(exp_distr(rng));
      using ResultType = decltype(res);
      return 
        res != std::numeric_limits<ResultType>::infinity()
        ? res
        : std::numeric_limits<ResultType>::max();
    };
  };

  //reality is fat-tailed and doesn't even owe you an expected value
  // (but we at least limit it max_message_bytes)
  auto constexpr pareto_shape = 1.0;
  auto pareto_rng = pareto_distribution(pareto_shape);

  //create a random hex string whose length is pareto distributed
  auto create_random_message = [&rng, &pareto_rng]() {
    //ensure that max_random_message_bytes can fit in EntropyType
    using EntropyType = decltype(rng());
    using ParetoType = decltype(pareto_rng());
    static_assert(
      static_cast<double>(max_random_message_bytes)
      <=
      static_cast<double>(std::numeric_limits<EntropyType>::max())
    );

    //sample pareto distribution for number of bytes of message
    auto pareto_sample = pareto_rng();
    
    //cap actual size using max_random_message_bytes
    using LargerType = decltype(ParetoType{} + max_random_message_bytes);
    auto const message_bytes = static_cast<EntropyType>(
      //find minimum in LargerType then safely convert result to EntropyType
      //  (a static_cast from double to integer that overflows is undefined behavior)
      std::min<LargerType>(pareto_sample, max_random_message_bytes)
    );
    
    //allocate message
    auto message = std::string(hex_digits_per_byte * message_bytes, '\0'); //might throw
    
    //fill message with hex characters using rng (efficiently) for entropy
    auto entropy = EntropyType();
    for (size_t i = 0; i < message_bytes; ++i) {
      auto constexpr bytes_per_entropy = sizeof(EntropyType);
      if (i % bytes_per_entropy == 0)
        entropy = rng();
      for (size_t j = 0; j < hex_digits_per_byte; ++j) { //can be unrolled
        auto hex_digit = entropy % 16;
        entropy >>= 4;
        message[2*i + j] = static_cast<char>(hex_digit < 10 ? '0' + hex_digit : 'A' + hex_digit-10);
      }
    }

    return message;
  };

  auto db = SQLite::Database(dbfile_, SQLite::OPEN_READWRITE);
  auto insert_query = SQLite::Statement(db, "INSERT INTO messages VALUES (NULL, ?, ?)");
  for (size_t i = 0; i < count; ++i) {
    auto message = create_random_message();
    insert_query.bind(1, static_cast<int64_t>(message.size()));
    insert_query.bindNoCopy(2, message);
    if (insert_query.exec() != 1)
      throw make_exception<Exception>(
        "Insertion failed at count: " + std::to_string(i) +
        " for message with size: " + std::to_string(message.size())
      );
    insert_query.reset();
  }
}

void SourceService::start(RecordCallback && cb, size_t log_frequency) {
  using namespace std::placeholders;
  start_thread(
    std::bind(&SourceService::work_loop, this, _1, _2, _3),
    std::move(cb),
    log_frequency
  );
}

void SourceService::work_loop(std::stop_token stop, RecordCallback && cb, size_t log_frequency) {
  //using namespace std::chrono_literals;
  //std::this_thread::sleep_for(1s);
  log("SourceService: work_loop started");
  auto db = SQLite::Database(dbfile_, SQLite::OPEN_READWRITE);
  auto query = SQLite::Statement(db, "SELECT id, message FROM messages");
  for (size_t i = 1; !stop.stop_requested() && query.executeStep(); ++i) {
    cb(stop, Record{query.getColumn(0).getInt(), query.getColumn(1).getString()});
    if (i % log_frequency == 0)
      log("SourceService: read " + std::to_string(i) + " messages");
  }
  log("SourceService: work_loop ended");
}