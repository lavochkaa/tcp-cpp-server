#pragma once

#include "storage/message.hpp"

#include <cstddef>
#include <string>
#include <vector>

void add_message(const std::string& from, const std::string& to, const std::string& text);
std::vector<Message> get_messages_for_user(const std::string& username);
std::vector<Message> get_sent_messages_for_user(const std::string& username);
std::vector<Message> get_dialog_messages(const std::string& user1, const std::string& user2);
std::vector<std::string> get_chat_partners_for_user(const std::string& username);
size_t get_total_messages_count();
