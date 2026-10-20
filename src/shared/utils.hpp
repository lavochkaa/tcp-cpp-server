#pragma once

#include <string>

class LineReader
{
public:
    explicit LineReader(int client_fd);
    bool read_line(std::string& message);

private:
    int client_fd_;
    std::string pending_;
};
