#ifndef KEY_SERVICE_HPP
#define KEY_SERVICE_HPP

#include <utility>
#include <mutex>

#include <cryptopp/xed25519.h>

#include "common.hpp"
#include "microservice.hpp"

class Key;

class KeyService : public Microservice {
  friend class Key;

  public:
    KeyService(size_t key_count);

    Key acquire_key();

  private:
    void release_key(std::string && public_key, CryptoPP::ed25519::Signer && signer);

    std::mutex mut_;
    std::deque<std::pair<std::string, CryptoPP::ed25519::Signer>> key_queue_;
};

#endif
