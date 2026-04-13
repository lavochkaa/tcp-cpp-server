#pragma once

#include <netinet/in.h>

void handle_client(int client_fd, sockaddr_in client_addr);
