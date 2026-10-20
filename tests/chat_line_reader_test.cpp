#include "shared/utils.hpp"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
    void send_text(int fd, const std::string& text)
    {
        size_t sent = 0;
        while (sent < text.size())
        {
            const ssize_t count = send(fd, text.data() + sent, text.size() - sent, 0);
            if (count < 0 && errno == EINTR)
                continue;
            assert(count > 0);
            sent += static_cast<size_t>(count);
        }
    }

    void test_coalesced_lines()
    {
        int sockets[2];
        const int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        assert(result == 0);
        const std::string input = "alice\n/hello\n";
        send_text(sockets[1], input);

        LineReader reader(sockets[0]);
        std::string line;
        assert(reader.read_line(line) && line == "alice");
        assert(reader.read_line(line) && line == "/hello");
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_fragmented_line()
    {
        int sockets[2];
        const int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        assert(result == 0);
        send_text(sockets[1], "/he");
        std::thread writer([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            send_text(sockets[1], "llo\r\n");
        });

        LineReader reader(sockets[0]);
        std::string line;
        assert(reader.read_line(line) && line == "/hello");
        writer.join();
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_max_length_crlf()
    {
        int sockets[2];
        const int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        assert(result == 0);
        const std::string input(1024, 'x');
        send_text(sockets[1], input + "\r");
        std::thread writer([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            send_text(sockets[1], "\n");
        });
        LineReader reader(sockets[0]);
        std::string line;
        assert(reader.read_line(line) && line == input);
        writer.join();
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_overlong_and_incomplete_lines()
    {
        int sockets[2];
        int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        assert(result == 0);
        const std::string input(1025, 'x');
        send_text(sockets[1], input);
        LineReader reader(sockets[0]);
        std::string line;
        assert(!reader.read_line(line));
        close(sockets[0]);
        close(sockets[1]);

        result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        assert(result == 0);
        send_text(sockets[1], "half");
        assert(shutdown(sockets[1], SHUT_WR) == 0);
        LineReader incomplete_reader(sockets[0]);
        assert(!incomplete_reader.read_line(line));
        close(sockets[0]);
        close(sockets[1]);
    }
}

int main()
{
    test_coalesced_lines();
    test_fragmented_line();
    test_max_length_crlf();
    test_overlong_and_incomplete_lines();
}
