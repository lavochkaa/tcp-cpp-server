#pragma once

#include <netinet/in.h>
#include <string>
#include "client.hpp"

void handle_http_client(int client_fd);
bool check_login_password(const std::string& login, const std::string& password);
