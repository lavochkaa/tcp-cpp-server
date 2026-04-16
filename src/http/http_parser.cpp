#include "http/http_parser.hpp"

#include <string>
#include <vector>
#include <utility>

std::string get_request_line(const std::string& request)
{
    size_t pos = request.find("\r\n");
    if (pos == std::string::npos)
        return request;

    return request.substr(0, pos);
}

std::string get_method(const std::string& request_line)
{
    size_t space = request_line.find(' ');
    if (space == std::string::npos)
        return request_line;

    return request_line.substr(0, space);
}

std::string get_path(const std::string& request_line)
{
    size_t first_space = request_line.find(' ');
    if (first_space == std::string::npos)
        return "";

    size_t second_space = request_line.find(' ', first_space + 1);
    if (second_space == std::string::npos)
        return "";

    return request_line.substr(first_space + 1, second_space - first_space - 1);
}

std::string get_body(const std::string& request)
{
    size_t pos = request.find("\r\n\r\n");
    if (pos == std::string::npos)
        return "";

    return request.substr(pos + 4);
}

std::string get_cookie_value(const std::string& request, const std::string& key)
{
    const std::string cookie_line = "Cookie: ";
    size_t start = request.find(cookie_line);

    if (start == std::string::npos)
        return "";

    size_t end = request.find("\r\n", start);
    std::string cookies = request.substr(start + cookie_line.size(), end - start - cookie_line.size());

    std::string prefix = key + "=";
    size_t value_start = cookies.find(prefix);

    if (value_start == std::string::npos)
        return "";

    value_start += prefix.size();

    size_t value_end = cookies.find(";", value_start);
    if (value_end == std::string::npos)
        value_end = cookies.size();

    return cookies.substr(value_start, value_end - value_start);
}

std::string get_form_value(const std::string& body, const std::string& key)
{
    std::string prefix = key + "=";
    size_t start = body.find(prefix);

    if (start == std::string::npos)
        return "";

    start += prefix.size();

    size_t end = body.find('&', start);
    if (end == std::string::npos)
        end = body.size();

    return body.substr(start, end - start);
}

std::vector<std::pair<std::string, std::string>> get_headers(const std::string& request)
{
    std::vector<std::pair<std::string, std::string>> headers;

    size_t line_start = request.find("\r\n");
    if (line_start == std::string::npos)
        return headers;

    line_start += 2;

    while (line_start < request.size())
    {
        size_t line_end = request.find("\r\n", line_start);
        if (line_end == std::string::npos || line_end == line_start)
            break;

        std::string line = request.substr(line_start, line_end - line_start);
        size_t colon = line.find(':');

        if (colon != std::string::npos)
        {
            std::string k = line.substr(0, colon);
            std::string v = line.substr(colon + 1);

            while (!v.empty() && v.front() == ' ')
                v.erase(v.begin());

            headers.push_back({k, v});
        }

        line_start = line_end + 2;
    }

    return headers;
}
