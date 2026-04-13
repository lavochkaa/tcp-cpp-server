#include "storage/session.hpp"
#include "storage/session_store.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace
{
    std::vector<Session> sessions;
    std::mutex sessions_mutex;
    int next_session_id = 1;
}

std::string create_session(const std::string& username)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);

    Session session;
    session.session_id = "session_" + std::to_string(next_session_id++);
    session.username = username;

    sessions.push_back(session);
    return session.session_id;
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
