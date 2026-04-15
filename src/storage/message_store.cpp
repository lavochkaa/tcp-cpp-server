#include "storage/message_store.hpp"

#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace
{
    std::vector<Message> messages;
    std::mutex messages_mutex;
}

void add_message(const std::string& from, const std::string& to, const std::string& text)
{
    std::lock_guard<std::mutex> lock(messages_mutex);

    Message msg;
    msg.from = from;
    msg.to = to;
    msg.text = text;

    messages.push_back(msg);
}

std::vector<Message> get_messages_for_user(const std::string& username)
{
    std::lock_guard<std::mutex> lock(messages_mutex);
    std::vector<Message> result;

    for (const Message& msg : messages)
    {
        if(msg.to == username)
            result.push_back(msg);
    }

    return result;
}

std::vector<Message> get_sent_messages_for_user(const std::string& username)
{
    std::lock_guard<std::mutex> lock(messages_mutex);
    std::vector<Message> result;

    for (const Message& msg : messages)
    {
        if (msg.from == username)
            result.push_back(msg);
    }

    return result;
}

std::vector<Message> get_dialog_messages(const std::string& user1, const std::string& user2)
{
    std::lock_guard<std::mutex> lock(messages_mutex);
    std::vector<Message> result;

    for (const Message& msg : messages)
    {
        bool first_to_second = (msg.from == user1 && msg.to == user2);
        bool second_to_first = (msg.from == user2 && msg.to == user1);

        if(first_to_second || second_to_first)
            result.push_back(msg);
    }

    return result;
}

std::vector<std::string> get_chat_partners_for_user(const std::string& username)
{
    std::lock_guard<std::mutex> lock(messages_mutex);
    std::set<std::string> partners;

    for (const Message& msg : messages)
    {
        if (msg.from == username && msg.to != username)
            partners.insert(msg.to);
        else if (msg.to == username && msg.from != username)
            partners.insert(msg.from);
    }

    return std::vector<std::string>(partners.begin(), partners.end());
}

size_t get_total_messages_count()
{
    std::lock_guard<std::mutex> lock(messages_mutex);
    return messages.size();
}
