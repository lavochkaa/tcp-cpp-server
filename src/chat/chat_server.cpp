#include "chat/chat_server.hpp"
#include "chat/client_manager.hpp"
#include "chat/client.hpp"
#include "chat/authorizate.hpp"
#include "shared/utils.hpp"
#include "storage/message_store.hpp"
#include "storage/session_store.hpp"

#include <iostream>
#include <string>
#include <vector>
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

    std::string active_chat_user;

    while (true)
    {
        std::string message;
        if (!recv_line(client_fd, message))
            break;
        if (message.empty())
            continue;

        if (message == "/help")
        {
            std::string help;
            help += "\nAvailable commands:\n";
            help += "/ip - Show current ip and port\n";
            help += "/hello - Show std tcp return hello\n";
            help += "/echo - Return your message\n";
            help += "/html - Show default HTML 'hello'\n";
            help += "/chat <username> - Open chat with user\n";
            help += "/back - Leave current chat dialog\n";
            help += "/sent_messages - Show sent messages\n";
            help += "/session_messages - Show incoming messages\n";
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

        else if (message == "/back")
        {
            if (active_chat_user.empty())
            {
                std::string fail = "You are not in chat mode.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
            }
            else
            {
                active_chat_user.clear();
                std::string ok = "Back to main menu.\n";
                send(client_fd, ok.c_str(), ok.size(), 0);
            }
        }

        else if (message.rfind("/chat ", 0) == 0)
        {
            std::string to_user = message.substr(6);

            if (to_user == client.username)
            {
                std::string fail = "You cannot send message to yourself.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
                continue;
            }

            if (to_user.empty())
            {
                std::string fail = "Usage: /chat <username>\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
                continue;
            }

            if (!is_user_online(to_user) && !is_session_active(to_user))
            {
                std::string fail = "User is not logged in.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
                continue;
            }

            std::vector<Message> dialog = get_dialog_messages(client.username, to_user);
            std::string dialog_preview = "\nChat with [" + to_user + "]:\n";

            if (dialog.empty())
            {
                dialog_preview += "No messages yet.\n";
            }
            else
            {
                for (const Message& msg : dialog)
                {
                    dialog_preview += "[" + msg.from + "]: " + msg.text + "\n";
                }
            }

            active_chat_user = to_user;
            send(client_fd, dialog_preview.c_str(), dialog_preview.size(), 0);
            std::string enter_message = "Enter messages now. Use /back to leave this chat.\n";
            send(client_fd, enter_message.c_str(), enter_message.size(), 0);
        }

        else if (message == "/sent_messages")
        {
            std::vector<Message> sent_messages = get_sent_messages_for_user(client.username);

            if (sent_messages.empty())
            {
                std::string no_messages = "No messages sent.\n";
                send(client_fd, no_messages.c_str(), no_messages.size(), 0);
            }

            else
            {
                std::string response = "\nYour sent messages:\n";

                for (const Message& msg : sent_messages)
                {
                    response += "To [" + msg.to + "]: " + msg.text + "\n";
                }

                send(client_fd, response.c_str(), response.size(), 0);
            }
            
        }
        
        else if (message == "/session_messages")
        {
            std::vector<Message> user_messages = get_messages_for_user(client.username);

            if (user_messages.empty())
            {
                std::string no_messages = "No messages.\n";
                send(client_fd, no_messages.c_str(), no_messages.size(), 0);
            }

            else
            {
                std::string response = "Your messages:\n";

                for (const Message& msg : user_messages)
                {
                    response += "From [" + msg.from + "]: " + msg.text + "\n"; 
                }

                send(client_fd, response.c_str(), response.size(), 0);
            }

        }

        else if (message == "/exit")
        {
            break;
        }

        else if (!active_chat_user.empty())
        {
            if (!is_user_online(active_chat_user) && !is_session_active(active_chat_user))
            {
                std::string fail = "User is no longer logged in.\n";
                send(client_fd, fail.c_str(), fail.size(), 0);
                active_chat_user.clear();
                continue;
            }

            std::string private_msg =
                "\nNew message from [" + client.username + "]: " + message + "\n";

            add_message(client.username, active_chat_user, message);

            if (is_user_online(active_chat_user))
                send_to_user(active_chat_user, private_msg);

            std::string local_echo = "[" + client.username + "]: " + message + "\n";
            send(client_fd, local_echo.c_str(), local_echo.size(), 0);
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
