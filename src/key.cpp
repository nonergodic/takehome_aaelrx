#include "key.hpp"

#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>

#include "key_service.hpp"

Key::~Key() {
  if (!public_key_.empty()) //if Key hasn't been moved out of
    service_.release_key(std::move(public_key_), std::move(signer_));
}

Key::Key(
  KeyService & service,
  std::string && public_key,
  CryptoPP::ed25519::Signer && signer
) :
    service_(service),
    public_key_(std::move(public_key)),
    signer_(std::move(signer)) {
}

std::string Key::sign(std::string const & message) const {
  auto signature_blob = std::string();
  CryptoPP::StringSource(
    message,
    true,
    new CryptoPP::SignerFilter(
      CryptoPP::NullRNG(),
      signer_,
      new CryptoPP::StringSink(signature_blob)
    )
  );
  auto signature = std::string();
  auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(signature));
  CryptoPP::StringSource(signature_blob, true, new CryptoPP::Redirector(encoder));
  //std::cout << "signature: " << encoder_str << " size: " << encoder_str.size() << std::endl;
  return signature;
};

bool Key::verify(std::string const & message, std::string const & signature) const {
  auto verifier = CryptoPP::ed25519::Verifier(signer_);

  std::string signature_blob;
  CryptoPP::StringSource ss(
    signature,
    true,
    new CryptoPP::HexDecoder(new CryptoPP::StringSink(signature_blob))
  );
  auto valid = false;
  CryptoPP::StringSource(
    signature_blob + message,
    true,
    new CryptoPP::SignatureVerificationFilter(
      verifier,
      new CryptoPP::ArraySink(
        reinterpret_cast<CryptoPP::byte*>(&valid),
        sizeof(valid)
      )
    )
  );

  return valid;
}