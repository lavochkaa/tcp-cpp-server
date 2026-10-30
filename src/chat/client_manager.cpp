#include "chat/client_manager.hpp"
#include "shared/utils.hpp"
#include "storage/session_store.hpp"

#include <vector>
#include <mutex>
#include <algorithm>
#include <memory>
#include <unistd.h>

namespace
{
    struct ClientEntry
    {
        Client client;
        std::shared_ptr<std::mutex> send_mutex;
    };

    std::vector<ClientEntry> clients;
    std::mutex clients_mutex;
}

std::mutex& username_claim_mutex()
{
    static std::mutex mutex;
    return mutex;
}

bool add_client(const Client& client)
{
    std::lock_guard<std::mutex> claim_lock(username_claim_mutex());
    if (is_session_active(client.username))
        return false;

    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const ClientEntry& entry : clients)
    {
        if (entry.client.username == client.username)
            return false;
    }
    clients.push_back({client, std::make_shared<std::mutex>()});
    return true;
}

void remove_client(int fd)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    clients.erase(
        std::remove_if(clients.begin(), clients.end(), 
            [fd](const ClientEntry& entry)
            {
                return entry.client.fd == fd;
            }),
        clients.end()
    );
}

bool send_to_user(const std::string& to_username, const std::string& text)
{
    int send_fd = -1;
    std::shared_ptr<std::mutex> send_mutex;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const ClientEntry& entry : clients)
        {
            if (entry.client.username == to_username)
            {
                send_fd = dup(entry.client.fd);
                send_mutex = entry.send_mutex;
                break;
            }
        }
    }

    if (send_fd < 0)
        return false;

    std::lock_guard<std::mutex> send_lock(*send_mutex);
    const bool sent = send_all(send_fd, text);
    close(send_fd);
    return sent;
}

bool is_user_online(const std::string& username)
{
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (const ClientEntry& entry : clients)
    {
        if (entry.client.username == username)
            return true;
    }

    return false;
}

std::vector<std::string> get_online_usernames()
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::vector<std::string> usernames;

    for (const ClientEntry& entry : clients)
    {
        usernames.push_back(entry.client.username);
    }

    return usernames;
}
