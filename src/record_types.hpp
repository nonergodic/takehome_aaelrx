#ifndef RECORD_TYPES_HPP
#define RECORD_TYPES_HPP

#include <string>
#include <vector>

struct Record {
  int id;
  std::string message;
};

struct SignedRecord {
  int id;
  std::string signature;
  std::string signer;
};

using SignedBatch = std::vector<SignedRecord>;

#endif
