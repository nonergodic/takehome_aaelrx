#include "startup.hpp"

#include <iostream>
#include <random>
#include <numeric>

#include <SQLiteCpp/SQLiteCpp.h>

#include <cryptopp/cryptlib.h>
#include <cryptopp/xed25519.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>

#include "common.hpp"
#include "crypto_sizes.hpp"

//maximum entropy of a random message (about 100MB - sqlite has an upper limit of ~1GB)
auto constexpr max_random_message_bytes = 1e8;

auto constexpr hex_digits_per_byte = size_t{2};

void create_and_fill_database(std::string const & dbfile, size_t count) {
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

  // signer.GetPrivateKey().Save(encoder);
  // auto private_key = encoder_str;
  // encoder_str.clear();
  
  
  

  // std::cout
  //   <<   " public key: " << public_key  << " size: " << public_key.size()
  //   << "\nprivate key: " << private_key << " size: " << private_key.size()
  //   << std::endl;
  
  // CryptoPP::ed25519::Signer signer2;
  // auto decoder = CryptoPP::StringSource(private_key, true, new CryptoPP::HexDecoder());
  // signer2.AccessPrivateKey().Load(decoder);
  // auto verifier2 = CryptoPP::ed25519::Verifier(signer2);

  // verifier2.GetPublicKey().Save(encoder);
  // auto public_key2 = encoder_str;
  // encoder_str.clear();

  // std::cout << "public_keys match: " << (public_key == public_key2) << std::endl;

  // auto verifier = CryptoPP::ed25519::Verifier(signer);
  // auto valid = false;
  // CryptoPP::StringSource(
  //   signature_blob + message,
  //   true,
  //   new CryptoPP::SignatureVerificationFilter(
  //     verifier,
  //     new CryptoPP::ArraySink(
  //       reinterpret_cast<CryptoPP::byte*>(&valid),
  //       sizeof(valid)
  //     )
  //   )
  // );
  // std::cout << "better be true: " << valid << std::endl;

  // message[0] = message[0] + 1;
  // CryptoPP::StringSource(
  //   signature_blob + message,
  //   true,
  //   new CryptoPP::SignatureVerificationFilter(
  //     verifier,
  //     new CryptoPP::ArraySink(
  //       reinterpret_cast<CryptoPP::byte*>(&valid),
  //       sizeof(valid)
  //     )
  //   )
  // );
  // std::cout << "better be false: " << valid << std::endl;

  auto create_ed25519_signer = []() {
    static auto prng = CryptoPP::AutoSeededRandomPool();
    auto signer = CryptoPP::ed25519::Signer();
    signer.AccessPrivateKey().GenerateRandom(prng);
    return signer;
  };

  auto signer = create_ed25519_signer();

  auto public_key = std::string();
  auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(public_key));
  auto verifier = CryptoPP::ed25519::Verifier(signer);
  verifier.GetPublicKey().Save(encoder);

  auto sign_message = [](auto const & signer, auto const & message) {
    //auto signature = std::string(signer.MaxSignatureLength(), '\0');
    auto signature_blob = std::string();
    CryptoPP::StringSource(
      message,
      true,
      new CryptoPP::SignerFilter(
        CryptoPP::NullRNG(),
        signer,
        new CryptoPP::StringSink(signature_blob)
      )
    );
    auto signature = std::string();
    auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(signature));
    CryptoPP::StringSource(signature_blob, true, new CryptoPP::Redirector(encoder));
    //std::cout << "signature: " << encoder_str << " size: " << encoder_str.size() << std::endl;
    return signature;
  };
  
  try {
    SQLite::Database db(dbfile, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
    // db.exec("DROP TABLE IF EXISTS messages");
    
    if (!SQLite::Statement(
          db,
          "SELECT name FROM sqlite_master WHERE type='table' AND name='messages'"
        ).executeStep()
    ) {
      db.exec(
        "CREATE TABLE messages ("
          "id INTEGER PRIMARY KEY, "
          "message TEXT, "
          "signature CHAR(" + std::to_string(hex_digits_per_byte * signature_bytes) + "), "
          "signer CHAR(" + std::to_string(hex_digits_per_byte * public_key_bytes) + ")"
        ")"
      );

      std::cout << "created messages table" << std::endl;
    }

    auto insert_query =
      SQLite::Statement(db, "INSERT INTO messages VALUES (NULL, ?, NULL, NULL)");
    auto message = create_random_message();
    insert_query.bindNoCopy(1, message);
    if (insert_query.exec() != 1)
      throw make_exception<Exception>("Insertion failed for message: " + message);
    std::cout << "inserted new message" << std::endl;

    auto update_query =
      SQLite::Statement(db, "UPDATE messages SET signature = ?, signer = ? WHERE id = ?");
    update_query.bindNoCopy(2, public_key);

    SQLite::Transaction transaction(db);
    auto update_select_query =
      SQLite::Statement(db, "SELECT id, message FROM messages WHERE signature IS NULL LIMIT " + std::to_string(count));
    std::cout << "selected messages to be signed" << std::endl;
    while (update_select_query.executeStep()) {
      auto id = update_select_query.getColumn(0).getInt();
      auto message = update_select_query.getColumn(1).getString();
      auto signature = sign_message(signer, message);

      update_query.bindNoCopy(1, signature);
      update_query.bind(3, id);

      if (update_query.exec() != 1)
        throw make_exception<Exception>("Update failed for id: " + std::to_string(id));
      update_query.reset();
    }
    transaction.commit();
    std::cout << "signed messages" << std::endl;

    auto review_select_query =
      SQLite::Statement(db, "SELECT * FROM messages");
    while (review_select_query.executeStep()) {
      std::cout
        << "\n       id: " << review_select_query.getColumn(0)
        << "\n  message: " << review_select_query.getColumn(1)
        << "\nsignature: " << review_select_query.getColumn(2)
        << "\n   signer: " << review_select_query.getColumn(3)
        << std::endl;
    }
    std::cout << "done" << std::endl;
  }
  catch (std::exception& e) {
    throw make_exception<Exception>(std::string("SQLite exception: ") + e.what());
  }
}

void create_private_keys(size_t){ // count) {

}