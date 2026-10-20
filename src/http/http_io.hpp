#pragma once

#include <string>

enum class HttpReadStatus { ok, bad_request, too_large, io_error };

HttpReadStatus read_http_request(int client_fd, std::string& request);
bool send_http_response(int client_fd, const std::string& response);
