#include "http/http_parser.hpp"

#include <string>
#include <vector>
#include <utility>
#include <cctype>

namespace
{
    bool ascii_iequals(const std::string& left, const std::string& right)
    {
        if (left.size() != right.size())
            return false;

        for (size_t i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) !=
                std::tolower(static_cast<unsigned char>(right[i])))
                return false;
        }
        return true;
    }

    std::string get_delimited_value(const std::string& fields, const std::string& key,
                                    char separator, bool skip_spaces)
    {
        size_t start = 0;
        while (start < fields.size())
        {
            size_t end = fields.find(separator, start);
            if (end == std::string::npos)
                end = fields.size();

            size_t name_start = start;
            if (skip_spaces)
            {
                while (name_start < end && fields[name_start] == ' ')
                    ++name_start;
            }

            size_t equals = fields.find('=', name_start);
            if (equals != std::string::npos && equals < end &&
                fields.compare(name_start, equals - name_start, key) == 0)
                return fields.substr(equals + 1, end - equals - 1);

            start = end + 1;
        }
        return "";
    }
}

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
    for (const auto& header : get_headers(request))
    {
        if (ascii_iequals(header.first, "Cookie"))
        {
            std::string value = get_delimited_value(header.second, key, ';', true);
            if (!value.empty())
                return value;
        }
    }
    return "";
}

std::string get_form_value(const std::string& body, const std::string& key)
{
    return get_delimited_value(body, key, '&', false);
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
