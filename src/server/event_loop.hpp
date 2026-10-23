#pragma once

#include <iostream>
#include <functional>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdexcept>
#include <unistd.h>

class EventLoop {
public:
    EventLoop(int server_fd, std::function<void(int, sockaddr_in)> on_new_client) {
        on_new_client_ = on_new_client;
        server_fd_ = server_fd;
        epfd_ = epoll_create1(0);

        if (epfd_ == -1) throw std::runtime_error("epoll_create1 failed");
    }
    ~EventLoop() {
        if (epfd_ != -1) close(epfd_);
    }

    void add_fd(int fd) {
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;

        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");
    }
    void run() {
        epoll_event events[64];
        add_fd(server_fd_); // начинаем слежку за серверным сокетом

        while (true) {
            int n = epoll_wait(epfd_, events, 64, -1); // n - колличество событий

            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;

                if (fd == server_fd_) {
                    sockaddr_in client_addr{};
                    socklen_t client_size = sizeof(client_addr);
                    int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_size);
                    if (client_fd == -1) {
                        std::cerr << "accept chat error\n";
                        continue;
                    }
                    on_new_client_(client_fd, client_addr);
                } else {
                    char buf[1024];
                    int bytes = recv(fd, buf, sizeof(buf), 0);
                    
                    if (bytes == 0) {
                        close(fd);
                        epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
                    }

                    std::cout << "Invoise: " << bytes << " bytes of fd = " << fd << std::endl;
                }
            }
        }
    }

private:
    int epfd_ = -1;
    int server_fd_ = -1;
    std::function<void(int, sockaddr_in)> on_new_client_;
};
