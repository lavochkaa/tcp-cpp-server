#include "chat/authorizate.hpp"
#include "chat/client_manager.hpp"
#include "storage/session_store.hpp"
#include "shared/utils.hpp"

#include <cassert>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int invalid[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, invalid) == 0);
    const std::string invalid_login = "ali\rce\n";
    assert(send(invalid[1], invalid_login.data(), invalid_login.size(), 0) ==
           static_cast<ssize_t>(invalid_login.size()));
    LineReader invalid_reader(invalid[0]);
    std::string invalid_username;
    assert(!authorize(invalid[0], invalid_username, invalid_reader));
    assert(invalid_username.empty());
    close(invalid[0]);
    close(invalid[1]);

    int connected[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, connected) == 0);
    const std::string login = "alice\n";
    assert(send(connected[1], login.data(), login.size(), 0) == static_cast<ssize_t>(login.size()));
    LineReader connected_reader(connected[0]);
    std::string connected_username;
    assert(authorize(connected[0], connected_username, connected_reader));
    assert(connected_username == "alice");
    char reply[33];
    assert(recv(connected[1], reply, sizeof(reply), MSG_WAITALL) == static_cast<ssize_t>(sizeof(reply)));
    assert(std::string(reply, sizeof(reply)) == "Login: Authorization successful!\n");

    int duplicate[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, duplicate) == 0);
    assert(send(duplicate[1], login.data(), login.size(), 0) == static_cast<ssize_t>(login.size()));
    LineReader duplicate_reader(duplicate[0]);
    std::string duplicate_username;
    assert(!authorize(duplicate[0], duplicate_username, duplicate_reader));
    assert(duplicate_username.empty());
    close(duplicate[0]);
    close(duplicate[1]);

    remove_client(connected[0]);
    close(connected[0]);
    close(connected[1]);

    const std::string bob_id = create_session("bob");
    assert(!bob_id.empty());
    int web_duplicate[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, web_duplicate) == 0);
    const std::string bob_login = "bob\n";
    assert(send(web_duplicate[1], bob_login.data(), bob_login.size(), 0) ==
           static_cast<ssize_t>(bob_login.size()));
    LineReader web_duplicate_reader(web_duplicate[0]);
    std::string web_duplicate_username;
    assert(!authorize(web_duplicate[0], web_duplicate_username, web_duplicate_reader));
    assert(web_duplicate_username.empty());
    close(web_duplicate[0]);
    close(web_duplicate[1]);
    remove_session(bob_id);

    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    close(sockets[1]);

    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0)
    {
        LineReader reader(sockets[0]);
        std::string username;
        _exit(authorize(sockets[0], username, reader) ? 1 : 0);
    }

    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    close(sockets[0]);
}
