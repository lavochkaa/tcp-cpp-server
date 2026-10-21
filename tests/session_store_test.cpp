#include "storage/session_store.hpp"

#include <cassert>
#include <string>

int main()
{
    const std::string alice_id = create_session("alice");
    assert(get_username_by_session("session_1").empty());
    assert(alice_id.size() == 64);
    assert(alice_id.find_first_not_of("0123456789abcdef") == std::string::npos);
    assert(get_username_by_session(alice_id) == "alice");

    const std::string bob_id = create_session("bob");
    assert(bob_id.size() == 64);
    assert(bob_id != alice_id);
    assert(get_username_by_session(bob_id) == "bob");

    remove_session(alice_id);
    assert(get_username_by_session(alice_id).empty());
    assert(get_username_by_session(bob_id) == "bob");
}
