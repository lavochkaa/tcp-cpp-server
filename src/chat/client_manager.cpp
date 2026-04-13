#include "chat/client_manager.hpp"

#include <vector>
#include <mutex>
#include <algorithm>
#include <sys/socket.h>

namespace
{
    std::vector<Client> clients;
    std::mutex clients_mutex;
}

void add_client(const Client& client)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(client);
}

void remove_client(int fd)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    clients.erase(
        std::remove_if(clients.begin(), clients.end(), 
            [fd](const Client& c)
            {
                return c.fd == fd;
            }),
        clients.end()
    );
}

bool send_to_user(const std::string& to_username, const std::string& text)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (const Client& c : clients)
    {
        if (c.username == to_username)
        {
            send(c.fd, text.c_str(), text.size(), 0);
            return true;
        }
    }
    return false;
}

bool is_user_online(const std::string& username)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (const Client& c : clients)
    {
        if (c.username == username)
            return true;
    }

    return false;
}

std::vector<std::string> get_online_usernames()
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::vector<std::string> usernames;

    for (const Client& c : clients)
    {
        usernames.push_back(c.username);
    }

    return usernames;
}
