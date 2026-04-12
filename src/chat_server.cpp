#include "chat_server.hpp"
#include "client_manager.hpp"
#include "client.hpp"
#include "authorizate.hpp"
#include "utils.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    std::cout << client.username << " logged in!\n";
    std::string welcome_msg = "Welcome, " + username + "!\n";
    send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);

    std::string main_menu = "\nEnter /help for show commands\n";
    send(client_fd, main_menu.c_str(), main_menu.size(), 0);

    while (true)
    {
        std::string message = recv_line(client_fd);
        if (message.empty())
            break;

        if (message == "/help")
        {
            std::string help;
            help += "\nAvailable commands:\n";
            help += "/ip - Show current ip and port\n";
            help += "/hello - Show std tcp return hello\n";
            help += "/echo - Return your message\n";
            help += "/html - Show default HTML 'hello'\n";
            help += "/send - Send message to user\n";
            help += "/help - Show available commands\n";
            help += "/exit - Disconnect from server\n";

            send(client_fd, help.c_str(), help.size(), 0);
        }

        else if (message == "/ip")
        {
            std::string responce = "\nYour IP is: ";
            responce += client_ip;
            responce += " and your port is: ";
            responce += std::to_string(ntohs(client_addr.sin_port));
            responce += "\n";
            send(client_fd, responce.c_str(), responce.size(), 0);
        }

        else if (message == "/hello")
        {
            std::string responce = "\nHello from the server!\n";
            send (client_fd, responce.c_str(), responce.size(), 0);
        }

        else if (message.rfind("/echo", 0) == 0)
        {
            std::string client_message = message.substr(6);
            std::cout << "Echo from " << client.username << ":" << client_message << "\n";
            std::string responce = "You said:" + client_message + "\n";
            send(client_fd, responce.c_str(), responce.size(), 0); 
        }

        else if (message == "/html")
        {
            std::string response =
                "\n<!DOCTYPE html>\n<html>\n<head>\n<title>My Web Server</title>\n</head>\n<body>\n<h1>Hello, World!</h1>\n<p>This is a simple HTML page served by the C++ server.</p>\n</body>\n</html>\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        else if (message.rfind("/send", 0) == 0)
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

        else if (message == "/exit")
        {
            break;
        }

        else
        {
            std::string response = "\nInvalid choice. Please try again.\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
    }

    remove_client(client_fd);
    close(client_fd);
}
