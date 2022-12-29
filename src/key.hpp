#ifndef KEY_HPP
#define KEY_HPP

#include <cryptopp/xed25519.h>

#include "common.hpp"

class KeyService;

class Key {
  friend class KeyService;

  public:
    Key(Key &&) = default;
    ~Key();

    std::string const & get_public_key() const {return public_key_;}

    std::string sign(std::string const & message) const;
    bool verify(std::string const & message, std::string const & signature) const;

  private:
    Key(KeyService & service, std::string && public_key, CryptoPP::ed25519::Signer && signer);

    KeyService & service_;
    std::string public_key_;
    CryptoPP::ed25519::Signer signer_;
};

#endif
