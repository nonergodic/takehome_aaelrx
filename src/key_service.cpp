#include "key_service.hpp"

#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
// #include <cryptopp/files.h>

#include "key.hpp"

KeyService::KeyService(size_t key_count) {
  auto prng = CryptoPP::AutoSeededRandomPool();

  for (size_t i = 0; i < key_count; ++i) {
    //generate signer (~= private key)
    auto signer = CryptoPP::ed25519::Signer();
    signer.AccessPrivateKey().GenerateRandom(prng);

    //calculate public key
    auto public_key = std::string();
    auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(public_key));
    auto verifier = CryptoPP::ed25519::Verifier(signer);
    verifier.GetPublicKey().Save(encoder);

    //store
    key_queue_.emplace_back(std::move(public_key), std::move(signer));
  }
}

Key KeyService::acquire_key() {
  log("KeyService: acquired key: " + key_queue_.front().first);
  auto lock = std::scoped_lock(mut_);
  assert(!key_queue_.empty()); //we currently assume that there is more keys than worker threads
  auto pair = std::move(key_queue_.front());
  key_queue_.pop_front();
  return Key{*this, std::move(pair.first), std::move(pair.second)};
}

void KeyService::release_key(std::string && public_key, CryptoPP::ed25519::Signer && signer) {
  log("KeyService: released key: " + public_key);
  auto lock = std::scoped_lock(mut_);
  key_queue_.emplace_back(std::move(public_key), std::move(signer));
}
