#include "utils.hpp"

#include <sys/socket.h>
#include <string>

std::string recv_line(int client_fd)
{
    char buffer[1024] = {0};

    int bytes = recv(client_fd,
                     buffer,
                     sizeof(buffer) - 1,
                     0);

    if (bytes <= 0)
        return "";

    buffer[bytes] = '\0';

    std::string message = buffer;

    while (!message.empty() &&
           (message.back() == '\n' ||
            message.back() == '\r'))
    {
        message.pop_back();
    }

    return message;
}