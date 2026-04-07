#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client.hpp"
#include "client_manager.hpp"

std::string recv_line(int client_fd)
{
    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0)
        return "";

    buffer[bytes_received] = '\0';
    std::string message = buffer;

    while (!message.empty() &&
           (message.back() == '\n' || message.back() == '\r'))
    {
        message.pop_back();
    }

    return message;
}

bool authorize(int client_fd, std::string& username)
{
    send(client_fd, "Login: ", 7, 0);
    std::string login = recv_line(client_fd);
    if (login.empty())
        return false;

    send(client_fd, "Password: ", 10, 0);
    std::string password = recv_line(client_fd);
    if (password.empty())
        return false;

    if ((login == "admin" && password == "admin") ||
        (login == "user" && password == "qwerty") ||
        (login == "test" && password == "123"))
    {
        username = login;
        send(client_fd, "Authorization successful!\n", 26, 0);
        return true;
    }

    send(client_fd, "Authorization failed!\n", 22, 0);
    return false;
}

void handle_client(int client_fd, sockaddr_in client_addr)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    std::cout << "Client connected: "
              << client_ip << ":" << ntohs(client_addr.sin_port) << "\n";

    std::string username;
    if (!authorize(client_fd, username))
    {
        close(client_fd);
        return;
    }

    Client client;
    client.fd = client_fd;
    client.username = username;
    add_client(client);

    std::string welcome_msg = "Welcome, " + username + "!\n";
    send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);

    while (true)
    {
        std::string main_menu;
        main_menu += "\n=== MAIN MENU ===\n";
        main_menu += "1. Show current ip and port\n";
        main_menu += "2. Show std tcp return hello\n";
        main_menu += "3. Return your message\n";
        main_menu += "4. Show default HTML 'hello'\n";
        main_menu += "5. Send message\n";
        main_menu += "0. Exit\n";
        main_menu += "Your choice: ";

        send(client_fd, main_menu.c_str(), main_menu.size(), 0);

        std::string message = recv_line(client_fd);
        if (message.empty())
            break;

        std::string response;

        if (message == "1")
        {
            response = "\nYour IP is: ";
            response += client_ip;
            response += " and your port is: ";
            response += std::to_string(ntohs(client_addr.sin_port));
            response += "\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        else if (message == "2")
        {
            response = "\nHello from the server!\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        else if (message == "3")
        {
            std::string ask = "Enter your message: ";
            send(client_fd, ask.c_str(), ask.size(), 0);

            std::string user_msg = recv_line(client_fd);
            if (user_msg.empty())
                break;

            response = "\n" + client.username + " said: " + user_msg + "\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        else if (message == "4")
        {
            response =
                "\n<!DOCTYPE html>\n<html>\n<head>\n<title>My Web Server</title>\n</head>\n<body>\n<h1>Hello, World!</h1>\n<p>This is a simple HTML page served by the C++ server.</p>\n</body>\n</html>\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        else if (message == "5")
        {
            send(client_fd, "Enter username: ", 16, 0);
            std::string to_user = recv_line(client_fd);
            if (to_user.empty())
                break;

            if (to_user == client.username)
            {
                std::string fail = "You cannot send message to yourself.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
                continue;
            }

            send(client_fd, "Send message: ", 14, 0);
            std::string user_msg = recv_line(client_fd);
            if (user_msg.empty())
                break;

            std::string private_msg =
                "\nNew message from [" + client.username + "]: " + user_msg + "\n";

            if (send_to_user(to_user, private_msg))
            {
                std::string ok = "Message sent!\n";
                send(client_fd, ok.c_str(), ok.size(), 0);
            }
            else
            {
                std::string fail = "User not found.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
            }
        }

        else if (message == "0")
        {
            break;
        }
        
        else
        {
            response = "\nInvalid choice. Please try again.\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
    }

    remove_client(client_fd);
    close(client_fd);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "socket error\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "bind error\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1)
    {
        std::cerr << "listen error\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";

    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_size = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_size);
        if (client_fd == -1)
        {
            std::cerr << "accept error\n";
            continue;
        }

        std::thread client_thread(handle_client, client_fd, client_addr);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}