#include "shared/utils.hpp"

#include <cerrno>
#include <sys/socket.h>
#include <string>

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
