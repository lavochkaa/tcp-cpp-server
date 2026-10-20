#include "http/http_io.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstring>
#include <poll.h>
#include <string>
#include <sys/socket.h>

namespace
{
    constexpr size_t kMaxHeaders = 16 * 1024;
    constexpr size_t kMaxRequest = 64 * 1024;

    bool header_name_equals(const std::string& line, size_t colon, const char* name)
    {
        const size_t length = std::strlen(name);
        if (colon != length)
            return false;
        for (size_t i = 0; i < length; ++i)
        {
            if (std::tolower(static_cast<unsigned char>(line[i])) != name[i])
                return false;
        }
        return true;
    }

    HttpReadStatus get_body_length(const std::string& request, size_t header_end,
                                   size_t& body_length)
    {
        size_t line_start = request.find("\r\n");
        if (line_start == std::string::npos || line_start > header_end)
            return HttpReadStatus::bad_request;
        line_start += 2;

        bool has_length = false;
        while (line_start < header_end)
        {
            const size_t line_end = request.find("\r\n", line_start);
            if (line_end == std::string::npos || line_end > header_end)
                return HttpReadStatus::bad_request;
            const std::string line = request.substr(line_start, line_end - line_start);
            const size_t colon = line.find(':');
            if (colon == std::string::npos)
                return HttpReadStatus::bad_request;

            if (header_name_equals(line, colon, "transfer-encoding"))
                return HttpReadStatus::bad_request;
            if (header_name_equals(line, colon, "content-length"))
            {
                if (has_length)
                    return HttpReadStatus::bad_request;
                has_length = true;
                size_t value_start = colon + 1;
                while (value_start < line.size() && line[value_start] == ' ')
                    ++value_start;
                if (value_start == line.size())
                    return HttpReadStatus::bad_request;
                const char* begin = line.data() + value_start;
                const char* end = line.data() + line.size();
                auto parsed = std::from_chars(begin, end, body_length);
                if (parsed.ec != std::errc() || parsed.ptr != end)
                    return HttpReadStatus::bad_request;
            }
            line_start = line_end + 2;
        }
        return HttpReadStatus::ok;
    }
}

HttpReadStatus read_http_request(int client_fd, std::string& request)
{
    request.clear();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    size_t total_length = 0;
    bool headers_complete = false;
    char buffer[4096];
    for (;;)
    {
        if (headers_complete && request.size() >= total_length)
        {
            request.resize(total_length);
            return HttpReadStatus::ok;
        }
        if (request.size() == kMaxRequest ||
            (!headers_complete && request.size() >= kMaxHeaders))
            return HttpReadStatus::too_large;

        const size_t remaining = (headers_complete ? total_length : kMaxHeaders)
                                 - request.size();
        const auto time_left = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (time_left.count() <= 0)
            return HttpReadStatus::io_error;
        pollfd ready{client_fd, POLLIN, 0};
        const int ready_count = poll(&ready, 1, static_cast<int>(time_left.count()));
        if (ready_count == 0)
            return HttpReadStatus::io_error;
        if (ready_count < 0)
        {
            if (errno == EINTR)
                continue;
            return HttpReadStatus::io_error;
        }
        const ssize_t count = recv(client_fd, buffer,
                                   std::min(sizeof(buffer), remaining), 0);
        if (count == 0)
            return HttpReadStatus::bad_request;
        if (count < 0)
        {
            if (errno == EINTR)
                continue;
            return HttpReadStatus::io_error;
        }
        request.append(buffer, static_cast<size_t>(count));

        if (!headers_complete)
        {
            const size_t header_end = request.find("\r\n\r\n");
            if (header_end == std::string::npos)
                continue;
            const size_t body_start = header_end + 4;
            if (body_start > kMaxHeaders)
                return HttpReadStatus::too_large;
            size_t body_length = 0;
            const HttpReadStatus status = get_body_length(request, header_end, body_length);
            if (status != HttpReadStatus::ok)
                return status;
            if (body_length > kMaxRequest - body_start)
                return HttpReadStatus::too_large;
            total_length = body_start + body_length;
            headers_complete = true;
        }
    }
}

bool send_http_response(int client_fd, const std::string& response)
{
#if defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)
    const int enabled = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0)
        return false;
#endif
    size_t sent = 0;
    while (sent < response.size())
    {
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        const ssize_t count = send(client_fd, response.data() + sent,
                                   response.size() - sent, flags);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return false;
        sent += static_cast<size_t>(count);
    }
    return true;
}
