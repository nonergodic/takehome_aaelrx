#ifndef CRYPTO_SIZES_HPP
#define CRYPTO_SIZES_HPP

#include "common.hpp"

auto constexpr signature_bytes = size_t{64};
auto constexpr public_key_bytes = size_t{44};
auto constexpr private_key_bytes = size_t{48};

#endif