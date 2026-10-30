#include "chat/client_manager.hpp"

#include <cassert>
#include <chrono>
#include <future>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    assert(add_client({sockets[0], "alice"}));
    assert(!add_client({sockets[0], "alice"}));

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

    int slow_sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, slow_sockets) == 0);
    int send_buffer_size = 1024;
    assert(setsockopt(slow_sockets[0], SOL_SOCKET, SO_SNDBUF,
                      &send_buffer_size, sizeof(send_buffer_size)) == 0);
    assert(add_client({slow_sockets[0], "slow"}));

    auto sender = std::async(std::launch::async, [&] {
        return send_to_user("slow", std::string(4 * 1024 * 1024, 'x'));
    });
    assert(sender.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);

    auto observer = std::async(std::launch::async, [] {
        return get_online_usernames();
    });
    const bool registry_available =
        observer.wait_for(std::chrono::milliseconds(500)) == std::future_status::ready;

    size_t received = 0;
    char drain_buffer[8192];
    while (received < 4 * 1024 * 1024)
    {
        const ssize_t count = recv(slow_sockets[1], drain_buffer,
                                   sizeof(drain_buffer), 0);
        assert(count > 0);
        received += static_cast<size_t>(count);
    }
    assert(sender.get());
    assert(observer.get().size() == 1);
    remove_client(slow_sockets[0]);
    close(slow_sockets[0]);
    close(slow_sockets[1]);
    assert(registry_available);
}
