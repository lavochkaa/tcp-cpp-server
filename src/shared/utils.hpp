#pragma once

#include <string>

bool send_all(int client_fd, const std::string& text);
bool is_valid_username(const std::string& username);

class LineReader
{
public:
    explicit LineReader(int client_fd);
    bool read_line(std::string& message);

private:
    int client_fd_;
    std::string pending_;
};
