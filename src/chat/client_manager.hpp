#pragma once

#include <vector>
#include <string>
#include "chat/client.hpp"

void add_client(const Client& client);
void remove_client(int fd);
bool send_to_user(const std::string& to_username, const std::string& text);
bool is_user_online(const std::string& username);
std::vector<std::string> get_online_usernames();
