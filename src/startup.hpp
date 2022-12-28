#ifndef STARTUP_HPP
#define STARTUP_HPP

#include "common.hpp"

void db_exists(std::string const & dbfile);

void create_and_fill_database(std::string const & dbfile, size_t record_count);

void create_private_keys(size_t key_count);

#endif //STARTUP_HPP
