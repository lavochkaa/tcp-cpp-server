#include "chat/client_manager.hpp"

#include <cassert>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    add_client({sockets[0], "alice"});

    assert(send_to_user("alice", "hello"));
    char buffer[5];
    assert(recv(sockets[1], buffer, sizeof(buffer), MSG_WAITALL) == 5);
    assert(std::string(buffer, sizeof(buffer)) == "hello");
    assert(!send_to_user("missing", "hello"));

    close(sockets[1]);
    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0)
        _exit(send_to_user("alice", "after disconnect") ? 1 : 0);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    remove_client(sockets[0]);
    close(sockets[0]);
}
