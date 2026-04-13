#pragma once

#include <string>
#include <vector>

std::string create_session(const std::string& username);
std::string get_username_by_session(const std::string& session_id);
void remove_session(const std::string& session_id);
bool is_session_active(const std::string& username);
std::vector<std::string> get_session_usernames();
