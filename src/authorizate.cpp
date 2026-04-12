#include "authorizate.hpp"
#include "utils.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

bool authorize(int client_fd, std::string& username)
{
    send(client_fd, "Login: ", 7, 0);
    std::string login = recv_line(client_fd);
    if (login.empty())
        return false;

    send(client_fd, "Password: ", 10, 0);
    std::string password = recv_line(client_fd);
    if (password.empty())
        return false;

    if ((login == "admin" && password == "admin") ||
        (login == "user" && password == "qwerty") ||
        (login == "test" && password == "123"))
    {
        username = login;
        send(client_fd, "Authorization successful!\n", 26, 0);
        return true;
    }

    send(client_fd, "Authorization failed!\n", 22, 0);
    return false;
}