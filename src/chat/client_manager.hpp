#pragma once

#include <vector>
#include <string>
#include <mutex>
#include "chat/client.hpp"

// Serializes username claims across TCP clients and web sessions.
std::mutex& username_claim_mutex();
bool add_client(const Client& client);
void remove_client(int fd);
bool send_to_user(const std::string& to_username, const std::string& text);
bool is_user_online(const std::string& username);
std::vector<std::string> get_online_usernames();
