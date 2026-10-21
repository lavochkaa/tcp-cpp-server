#include "chat/authorizate.hpp"
#include "chat/client_manager.hpp"
#include "storage/session_store.hpp"
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
    if (!reader.read_line(login) || login.empty())
        return false;

    if (is_user_online(login) || is_session_active(login))
    {
        send_all(client_fd, "User already logged in!\n");
        return false;
    }

    username = login;
    return send_all(client_fd, "Authorization successful!\n");
}
