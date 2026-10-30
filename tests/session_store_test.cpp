#include "storage/session_store.hpp"
#include "chat/client_manager.hpp"

#include <cassert>
#include <atomic>
#include <string>
#include <thread>

int main()
{
    const std::string alice_id = create_session("alice");
    assert(get_username_by_session("session_1").empty());
    assert(alice_id.size() == 64);
    assert(alice_id.find_first_not_of("0123456789abcdef") == std::string::npos);
    assert(get_username_by_session(alice_id) == "alice");
    assert(create_session("alice").empty());
    assert(get_session_usernames().size() == 1);

    const std::string bob_id = create_session("bob");
    assert(bob_id.size() == 64);
    assert(bob_id != alice_id);
    assert(get_username_by_session(bob_id) == "bob");

    remove_session(alice_id);
    assert(get_username_by_session(alice_id).empty());
    assert(get_username_by_session(bob_id) == "bob");

    const std::string new_alice_id = create_session("alice");
    assert(new_alice_id.size() == 64);
    assert(new_alice_id != alice_id);

    for (int attempt = 0; attempt < 20; ++attempt)
    {
        std::atomic<int> ready{0};
        std::atomic<bool> start{false};
        bool chat_claimed = false;
        std::string web_session;
        std::thread chat([&] {
            ++ready;
            while (!start.load()) std::this_thread::yield();
            chat_claimed = add_client({100 + attempt, "racer"});
        });
        std::thread web([&] {
            ++ready;
            while (!start.load()) std::this_thread::yield();
            web_session = create_session("racer");
        });
        while (ready.load() != 2) std::this_thread::yield();
        start = true;
        chat.join();
        web.join();

        assert(chat_claimed != !web_session.empty());
        if (chat_claimed)
            remove_client(100 + attempt);
        else
            remove_session(web_session);
    }
}
