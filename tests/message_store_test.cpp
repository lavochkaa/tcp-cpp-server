#include "storage/message_store.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{
    bool contains_username(const std::vector<std::string>& usernames,
                           const std::string& username)
    {
        return std::find(usernames.begin(), usernames.end(), username) != usernames.end();
    }

    bool contains_message(const std::vector<Message>& messages,
                          const std::string& from,
                          const std::string& to,
                          const std::string& text)
    {
        for (const Message& message : messages)
        {
            if (message.from == from && message.to == to && message.text == text)
                return true;
        }
        return false;
    }
}

int main()
{
    assert(get_total_messages_count() == 0);

    add_message("alice", "bob", "hi bob");
    add_message("bob", "alice", "hi alice");
    add_message("alice", "carol", "hi carol");
    add_message("carol", "bob", "private");

    assert(get_total_messages_count() == 4);

    const std::vector<Message> alice_incoming = get_messages_for_user("alice");
    assert(alice_incoming.size() == 1);
    assert(contains_message(alice_incoming, "bob", "alice", "hi alice"));

    const std::vector<Message> alice_sent = get_sent_messages_for_user("alice");
    assert(alice_sent.size() == 2);
    assert(contains_message(alice_sent, "alice", "bob", "hi bob"));
    assert(contains_message(alice_sent, "alice", "carol", "hi carol"));

    assert(get_messages_for_user("dave").empty());
    assert(get_sent_messages_for_user("dave").empty());

    const std::vector<Message> alice_bob = get_dialog_messages("alice", "bob");
    assert(alice_bob.size() == 2);
    assert(contains_message(alice_bob, "alice", "bob", "hi bob"));
    assert(contains_message(alice_bob, "bob", "alice", "hi alice"));
    assert(get_dialog_messages("alice", "dave").empty());

    const std::vector<std::string> alice_partners = get_chat_partners_for_user("alice");
    assert(alice_partners.size() == 2);
    assert(contains_username(alice_partners, "bob"));
    assert(contains_username(alice_partners, "carol"));

    const std::vector<std::string> carol_partners = get_chat_partners_for_user("carol");
    assert(carol_partners.size() == 2);
    assert(contains_username(carol_partners, "alice"));
    assert(contains_username(carol_partners, "bob"));

    assert(get_chat_partners_for_user("dave").empty());
}
