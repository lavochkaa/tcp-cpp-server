#include "shared/utils.hpp"

#include <sys/socket.h>
#include <string>

bool recv_line(int client_fd, std::string& message)
{
    char buffer[1024] = {0};

    int bytes = recv(client_fd,
                     buffer,
                     sizeof(buffer) - 1,
                     0);

    if (bytes <= 0)
    {
        message.clear();
        return false;
    }

    buffer[bytes] = '\0';

    message = buffer;

    while (!message.empty() &&
           (message.back() == '\n' ||
            message.back() == '\r'))
    {
        message.pop_back();
    }

    return true;
}
