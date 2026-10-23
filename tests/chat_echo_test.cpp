#include "chat/chat_server.hpp"
#include "shared/utils.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    void test_ip_command_reports_peer_address()
    {
        const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(listen_fd >= 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        assert(bind(listen_fd, reinterpret_cast<sockaddr*>(&address),
                    sizeof(address)) == 0);
        assert(listen(listen_fd, 1) == 0);

        socklen_t address_length = sizeof(address);
        assert(getsockname(listen_fd, reinterpret_cast<sockaddr*>(&address),
                           &address_length) == 0);

        const int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(client_fd >= 0);
        assert(connect(client_fd, reinterpret_cast<sockaddr*>(&address),
                       sizeof(address)) == 0);

        const int server_fd = accept(listen_fd, nullptr, nullptr);
        assert(server_fd >= 0);
        close(listen_fd);

        sockaddr_in client_address{};
        socklen_t client_address_length = sizeof(client_address);
        assert(getsockname(client_fd, reinterpret_cast<sockaddr*>(&client_address),
                           &client_address_length) == 0);

        const pid_t child = fork();
        assert(child >= 0);
        if (child == 0)
        {
            close(client_fd);
            handle_client(server_fd, sockaddr_in{});
            _exit(0);
        }

        close(server_fd);
        assert(send_all(client_fd, "alice\n/ip\n/exit\n"));
        assert(shutdown(client_fd, SHUT_WR) == 0);

        std::string replies;
        char buffer[256];
        ssize_t received;
        while ((received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0)
            replies.append(buffer, static_cast<size_t>(received));
        assert(received == 0);
        close(client_fd);

        int status = 0;
        assert(waitpid(child, &status, 0) == child);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

        const std::string expected =
            "Your IP is: 127.0.0.1 and your port is: " +
            std::to_string(ntohs(client_address.sin_port)) + "\n";
        assert(replies.find(expected) != std::string::npos);
    }
}

int main()
{
    test_ip_command_reports_peer_address();

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
