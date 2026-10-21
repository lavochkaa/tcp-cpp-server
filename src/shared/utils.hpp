#pragma once

#include <string>

bool send_all(int client_fd, const std::string& text);

class LineReader
{
public:
    explicit LineReader(int client_fd);
    bool read_line(std::string& message);

private:
    int client_fd_;
    std::string pending_;
};
