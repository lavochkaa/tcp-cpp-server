#include "chat/authorizate.hpp"
#include "chat/client_manager.hpp"
#include "shared/utils.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

bool authorize(int client_fd, std::string& username, LineReader& reader)
{
    if (!send_all(client_fd, "Login: "))
        return false;
    std::string login;
    if (!reader.read_line(login))
        return false;
    if (!is_valid_username(login))
    {
        send_all(client_fd, "Invalid username.\n");
        return false;
    }

    if (!add_client({client_fd, login}))
    {
        send_all(client_fd, "User already logged in!\n");
        return false;
    }

    username = login;
    if (!send_all(client_fd, "Authorization successful!\n"))
    {
        remove_client(client_fd);
        username.clear();
        return false;
    }
    return true;
}
