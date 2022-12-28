// Secp256k1
//  https://www.cryptopp.com/wiki/Elliptic_Curve_Digital_Signature_Algorithm
//  (+) crypto-canonical (BTC & EVM native, also natively supported on Solana)
//  (-) requires randomness when signing
//
// ed25519
//  https://en.wikipedia.org/wiki/EdDSA
//  https://www.cryptopp.com/wiki/Ed25519
//  https://blog.peterruppel.de/ed25519-for-ssh/
//  (+) does not require randomness for signing
//  (+) supposedly 20x-30x faster than secp256k1 (see https://www.cryptopp.com/wiki/Ed25519#Benchmarks)
//  (-) bad for large messages
//  (~) verification natively supported on Solana, possible on EVM (see https://ethresear.ch/t/verify-ed25519-signatures-cheaply-on-eth-using-zk-snarks/13139)

#include <iostream>

#include "startup.hpp"

//int main(int argc, char** argv) {
int main() {

  try {
    create_and_fill_database("test.db", 2);
  }
  catch (Exception const & e) {
    std::cout << "caught exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}