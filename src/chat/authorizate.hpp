#pragma once

#include <string>

class LineReader;

bool authorize(int client_fd, std::string& username, LineReader& reader);
