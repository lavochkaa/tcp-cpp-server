#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http/http_server.hpp"
#include "chat/chat_server.hpp"
#include "server/thread_pool.hpp"

void run_http_server(int http_fd, ThreadPool& pool)
{
    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof(client_addr);

        int client_fd = accept(http_fd, (sockaddr*)&client_addr, &client_size);
        if (client_fd == -1)
        {
            std::cerr << "accept http error\n";
            continue;
        }

        pool.submit([client_fd] {
            handle_http_client(client_fd);
        });
    }
}

void run_chat_server(int chat_fd, ThreadPool& pool)
{
    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof(client_addr);

        int client_fd = accept(chat_fd, (sockaddr*)&client_addr, &client_size);
        if (client_fd == -1)
        {
            std::cerr << "accept chat error\n";
            continue;
        }

        pool.submit([client_fd, client_addr] {
            handle_client(client_fd, client_addr);
        });
    }
}

int main()
{
    int http_fd = socket(AF_INET, SOCK_STREAM, 0);
    int chat_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (http_fd == -1 || chat_fd == -1)
    {
        std::cerr << "socket error\n";
        return 1;
    }

    int opt = 1;
    setsockopt(http_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(chat_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in http_addr{};
    http_addr.sin_family = AF_INET;
    http_addr.sin_port = htons(8080);
    http_addr.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in chat_addr{};
    chat_addr.sin_family = AF_INET;
    chat_addr.sin_port = htons(8081);
    chat_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(http_fd, (sockaddr*)&http_addr, sizeof(http_addr)) == -1)
    {
        std::cerr << "bind http error\n";
        close(http_fd);
        return 1;
    }

    if(bind(chat_fd, (sockaddr*)&chat_addr, sizeof(chat_addr)) == -1)
    {
        std::cerr << "bind chat error\n";
        close(chat_fd);
        return 1;
    }

    if (listen(http_fd, 5) == -1)
    {
        std::cerr << "listen http error\n";
        close(http_fd);
        return 1;
    }

    if (listen(chat_fd, 5) == -1)
    {
        std::cerr << "listen chat error\n";
        close(chat_fd);
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";
    std::cout << "Server is listening on port 8081...\n";

    ThreadPool pool(8);

    std::thread http_thread(run_http_server, http_fd, std::ref(pool));
    std::thread chat_thread(run_chat_server, chat_fd, std::ref(pool));

    http_thread.join();
    chat_thread.join();

    close(http_fd);
    close(chat_fd);

    return 0;
}
