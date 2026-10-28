#include "storage/session.hpp"
#include "storage/session_store.hpp"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
    std::vector<Session> sessions;
    std::mutex sessions_mutex;

    std::string generate_session_id()
    {
        std::array<unsigned char, 32> bytes{};
        int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return "";

        size_t filled = 0;
        while (filled < bytes.size())
        {
            ssize_t count = read(fd, bytes.data() + filled, bytes.size() - filled);
            if (count < 0 && errno == EINTR)
                continue;
            if (count <= 0)
            {
                close(fd);
                return "";
            }
            filled += static_cast<size_t>(count);
        }
        close(fd);

        const char hex[] = "0123456789abcdef";
        std::string id;
        id.reserve(bytes.size() * 2);
        for (unsigned char byte : bytes)
        {
            id += hex[byte >> 4];
            id += hex[byte & 0x0f];
        }
        return id;
    }
}

std::string create_session(const std::string& username)
{
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        std::string id = generate_session_id();
        if (id.empty())
            return "";

        std::lock_guard<std::mutex> lock(sessions_mutex);
        bool duplicate = false;
        for (const Session& session : sessions)
        {
            if (session.username == username)
                return "";
            if (session.session_id == id)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        Session session;
        session.session_id = id;
        session.username = username;

        sessions.push_back(session);
        return id;
    }
    return "";
}

std::string get_username_by_session(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);

    for (const Session& session : sessions)
    {
        if (session.session_id == session_id)
            return session.username;
    }

    return "";
}

void remove_session(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (sessions[i].session_id == session_id)
        {
            sessions.erase(sessions.begin() + i);
            return;
        }
    }
}

bool is_session_active(const std::string& username)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);

    for (const Session& session : sessions)
    {
        if (session.username == username)
            return true;
    }

    return false;
}

std::vector<std::string> get_session_usernames()
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    std::vector<std::string> usernames;

    for (const Session& session : sessions)
    {
        usernames.push_back(session.username);
    }

    return usernames;
}
