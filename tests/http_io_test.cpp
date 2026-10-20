#include "http/http_io.hpp"

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

    void test_fragmented_post()
    {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        const std::string headers = "POST /login HTTP/1.1\r\nHost: localhost\r\n"
                                    "Content-Length: 11\r\n\r\n";
        send_text(sockets[1], headers);
        std::thread writer([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            send_text(sockets[1], "login=alice");
        });
        std::string request;
        assert(read_http_request(sockets[0], request) == HttpReadStatus::ok);
        assert(request == headers + "login=alice");
        writer.join();
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_header_only_request()
    {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        const std::string input = "GET / HTTP/1.1\r\n\r\n";
        send_text(sockets[1], input);
        std::string request;
        assert(read_http_request(sockets[0], request) == HttpReadStatus::ok);
        assert(request == input);
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_invalid_and_oversized_requests()
    {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        send_text(sockets[1], "POST / HTTP/1.1\r\nContent-Length: 2\r\n"
                              "Content-Length: 3\r\n\r\nabc");
        std::string request;
        assert(read_http_request(sockets[0], request) == HttpReadStatus::bad_request);
        close(sockets[0]);
        close(sockets[1]);

        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        send_text(sockets[1], "POST / HTTP/1.1\r\nContent-Length: 65536\r\n\r\n");
        assert(read_http_request(sockets[0], request) == HttpReadStatus::too_large);
        close(sockets[0]);
        close(sockets[1]);
    }

    void test_complete_response()
    {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        const std::string response(1024 * 1024, 'x');
        std::string received;
        std::thread reader([&] {
            char buffer[4096];
            while (received.size() < response.size())
            {
                const ssize_t count = recv(sockets[1], buffer, sizeof(buffer), 0);
                assert(count > 0);
                received.append(buffer, static_cast<size_t>(count));
            }
        });
        assert(send_http_response(sockets[0], response));
        reader.join();
        assert(received == response);
        close(sockets[0]);
        close(sockets[1]);
    }
}

int main()
{
    test_fragmented_post();
    test_header_only_request();
    test_invalid_and_oversized_requests();
    test_complete_response();
}
