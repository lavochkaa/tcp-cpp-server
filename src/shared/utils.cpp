#include "shared/utils.hpp"

#include <cerrno>
#include <sys/socket.h>
#include <string>

bool send_all(int client_fd, const std::string& text)
{
#if defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)
    const int enabled = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0)
        return false;
#endif

    size_t sent = 0;
    while (sent < text.size())
    {
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        const ssize_t count = send(client_fd, text.data() + sent, text.size() - sent, flags);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return false;
        sent += static_cast<size_t>(count);
    }
    return true;
}

LineReader::LineReader(int client_fd) : client_fd_(client_fd) {}

bool LineReader::read_line(std::string& message)
{
    constexpr size_t kMaxLineLength = 1024;
    char buffer[1024];
    for (;;)
    {
        const size_t newline = pending_.find('\n');
        if (newline != std::string::npos)
        {
            message = pending_.substr(0, newline);
            pending_.erase(0, newline + 1);
            if (!message.empty() && message.back() == '\r')
                message.pop_back();
            if (message.size() > kMaxLineLength)
            {
                message.clear();
                return false;
            }
            return true;
        }

        if (pending_.size() > kMaxLineLength &&
            !(pending_.size() == kMaxLineLength + 1 && pending_.back() == '\r'))
        {
            message.clear();
            return false;
        }

        const ssize_t bytes = recv(client_fd_, buffer, sizeof(buffer), 0);
        if (bytes < 0 && errno == EINTR)
            continue;
        if (bytes <= 0)
        {
            message.clear();
            return false;
        }
        pending_.append(buffer, static_cast<size_t>(bytes));
    }
}
