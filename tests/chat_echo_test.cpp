#include "chat/chat_server.hpp"
#include "shared/utils.hpp"

#include <cassert>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0)
    {
        close(sockets[1]);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        handle_client(sockets[0], address);
        _exit(0);
    }

    close(sockets[0]);
    const std::string commands = "alice\n/echo\n/echo hi\n/echofoo\n/hello\n/exit\n";
    assert(send_all(sockets[1], commands));
    assert(shutdown(sockets[1], SHUT_WR) == 0);

    std::string replies;
    char buffer[256];
    ssize_t received;
    while ((received = recv(sockets[1], buffer, sizeof(buffer), 0)) > 0)
        replies.append(buffer, static_cast<size_t>(received));
    assert(received == 0);
    close(sockets[1]);

    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(replies.find("You said:\n") != std::string::npos);
    assert(replies.find("You said:hi\n") != std::string::npos);
    assert(replies.find("Invalid choice. Please try again.") != std::string::npos);
    assert(replies.find("Hello from the server!") != std::string::npos);
}
