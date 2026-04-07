#pragma once

#include <string>
#include "client.hpp"

void add_client(const Client& client);
void remove_client(int fd);
bool send_to_user(const std::string& to_username, const std::string& text);
