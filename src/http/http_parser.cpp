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

    int hex_digit_value(char ch)
    {
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        return -1;
    }

    std::string url_decode(const std::string& value, bool plus_as_space)
    {
        std::string decoded;
        decoded.reserve(value.size());

        for (size_t i = 0; i < value.size(); ++i)
        {
            if (plus_as_space && value[i] == '+')
            {
                decoded += ' ';
            }
            else if (value[i] == '%' && i + 2 < value.size())
            {
                const int high = hex_digit_value(value[i + 1]);
                const int low = hex_digit_value(value[i + 2]);
                if (high >= 0 && low >= 0)
                {
                    decoded += static_cast<char>(high * 16 + low);
                    i += 2;
                }
                else
                {
                    decoded += value[i];
                }
            }
            else
            {
                decoded += value[i];
            }
        }

        return decoded;
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
    return url_decode(get_delimited_value(body, key, '&', false), true);
}

std::string encode_path_segment(const std::string& value)
{
    constexpr char hex[] = "0123456789ABCDEF";
    std::string encoded;
    for (unsigned char ch : value)
    {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
            ch == '.' || ch == '~')
        {
            encoded += static_cast<char>(ch);
        }
        else
        {
            encoded += '%';
            encoded += hex[ch >> 4];
            encoded += hex[ch & 0x0F];
        }
    }
    return encoded;
}

std::string decode_path_segment(const std::string& value)
{
    return url_decode(value, false);
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
