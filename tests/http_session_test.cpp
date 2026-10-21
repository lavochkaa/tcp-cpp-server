#include "http/http_server.hpp"
#include "shared/utils.hpp"

#include <cassert>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
    std::string exchange(const std::string& request)
    {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        std::thread server([fd = sockets[0]] { handle_http_client(fd); });
        assert(send_all(sockets[1], request));
        assert(shutdown(sockets[1], SHUT_WR) == 0);

        std::string response;
        char buffer[4096];
        ssize_t count;
        while ((count = recv(sockets[1], buffer, sizeof(buffer), 0)) > 0)
            response.append(buffer, static_cast<size_t>(count));
        assert(count == 0);
        close(sockets[1]);
        server.join();
        return response;
    }

    std::string chat_request(const std::string& session_id)
    {
        return "GET /chat HTTP/1.1\r\nHost: localhost\r\nCookie: session_id=" +
               session_id + "\r\n\r\n";
    }
}

int main()
{
    const std::string login = exchange("POST /login HTTP/1.1\r\nHost: localhost\r\n"
                                       "Content-Length: 11\r\n\r\nlogin=alice");
    assert(login.find("HTTP/1.1 302 Found") == 0);
    const std::string cookie_header = "Set-Cookie: session_id=";
    const size_t start = login.find(cookie_header);
    assert(start != std::string::npos);
    const size_t id_start = start + cookie_header.size();
    const size_t id_end = login.find("\r\n", id_start);
    assert(id_end != std::string::npos);
    const std::string session_id = login.substr(id_start, id_end - id_start);
    assert(session_id.size() == 64);

    assert(exchange(chat_request("session_1")).find("HTTP/1.1 302 Found") == 0);
    assert(exchange(chat_request(session_id)).find("HTTP/1.1 200 OK") == 0);

    const std::string logout = "GET /logout HTTP/1.1\r\nHost: localhost\r\n"
                               "Cookie: session_id=" + session_id + "\r\n\r\n";
    assert(exchange(logout).find("HTTP/1.1 302 Found") == 0);
    assert(exchange(chat_request(session_id)).find("HTTP/1.1 302 Found") == 0);
}
