#include "chat/authorizate.hpp"
#include "chat/client_manager.hpp"
#include "storage/session_store.hpp"
#include "shared/utils.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

bool authorize(int client_fd, std::string& username)
{
    send(client_fd, "Login: ", 7, 0);
    std::string login;
    if (!recv_line(client_fd, login) || login.empty())
        return false;

    if (is_user_online(login) || is_session_active(login))
    {
        send(client_fd, "User already logged in!\n", 24, 0);
        return false;
    }

    username = login;
    send(client_fd, "Authorization successful!\n", 26, 0);
    return true;
}
