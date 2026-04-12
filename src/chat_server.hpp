#pragma once

#include <netinet/in.h>
#include <string>
#include "client.hpp"

void handle_client(int client_fd, sockaddr_in client_addr);