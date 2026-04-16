#pragma once

#include <string>
#include <vector>
#include <utility>

std::string get_request_line(const std::string& request);
std::string get_method(const std::string& request_line);
std::string get_path(const std::string& request_line);
std::string get_body(const std::string& request);
std::string get_cookie_value(const std::string& request, const std::string& key);
std::string get_form_value(const std::string& body, const std::string& key);
std::vector<std::pair<std::string, std::string>> get_headers(const std::string& request);
