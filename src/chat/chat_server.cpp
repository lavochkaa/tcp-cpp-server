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
    LineReader reader(client_fd);
    if (!authorize(client_fd, username, reader))
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
    std::string main_menu = "\nEnter /help for show commands\n";
    if (!send_all(client_fd, welcome_msg) || !send_all(client_fd, main_menu))
    {
        remove_client(client_fd);
        close(client_fd);
        return;
    }

    std::string active_chat_user;

    while (true)
    {
        std::string message;
        if (!reader.read_line(message))
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
            help += "/inspect - Show collected server info\n";
            help += "/who - Show all loggined users\n";
            help += "/sent_messages - Show sent messages\n";
            help += "/session_messages - Show incoming messages\n";
            help += "/help - Show available commands\n";
            help += "/exit - Disconnect from server\n";

            if (!send_all(client_fd, help)) break;
        }

        else if (message == "/ip")
        {
            std::string responce = "\nYour IP is: ";
            responce += client_ip;
            responce += " and your port is: ";
            responce += std::to_string(ntohs(client_addr.sin_port));
            responce += "\n";
            if (!send_all(client_fd, responce)) break;
        }

        else if (message == "/hello")
        {
            std::string responce = "\nHello from the server!\n";
            if (!send_all(client_fd, responce)) break;
        }

        else if (message.rfind("/echo", 0) == 0)
        {
            std::string client_message = message.substr(6);
            std::cout << "Echo from " << client.username << ":" << client_message << "\n";
            std::string responce = "You said:" + client_message + "\n";
            if (!send_all(client_fd, responce)) break;
        }

        else if (message == "/html")
        {
            std::string response =
                "\n<!DOCTYPE html>\n<html>\n<head>\n<title>My Web Server</title>\n</head>\n<body>\n<h1>Hello, World!</h1>\n<p>This is a simple HTML page served by the C++ server.</p>\n</body>\n</html>\n";
            if (!send_all(client_fd, response)) break;
        }

        else if (message == "/back")
        {
            if (active_chat_user.empty())
            {
                std::string fail = "You are not in chat mode.\n";
                if (!send_all(client_fd, fail)) break;
            }
            else
            {
                active_chat_user.clear();
                std::string ok = "Back to main menu.\n";
                if (!send_all(client_fd, ok)) break;
            }
        }

        else if (message == "/inspect")
        {
            std::vector<std::string> online_users = get_online_usernames();
            std::vector<std::string> session_users = get_session_usernames();
            std::vector<std::string> chat_partners = get_chat_partners_for_user(client.username);

            std::string response = "\nServer inspect:\n";
            response += "username: " + client.username + "\n";
            response += "client fd: " + std::to_string(client.fd) + "\n";
            response += "client ip: " + std::string(client_ip) + "\n";
            response += "client port: " + std::to_string(ntohs(client_addr.sin_port)) + "\n";
            response += "server port: 8081\n";
            response += "active chat user: ";
            response += active_chat_user.empty() ? "(none)\n" : active_chat_user + "\n";
            response += "online terminal users: " + std::to_string(online_users.size()) + "\n";

            for (const std::string& username : online_users)
                response += "  - " + username + "\n";

            response += "active web sessions: " + std::to_string(session_users.size()) + "\n";

            for (const std::string& username : session_users)
                response += "  - " + username + "\n";

            response += "incoming messages: " + std::to_string(get_messages_for_user(client.username).size()) + "\n";
            response += "sent messages: " + std::to_string(get_sent_messages_for_user(client.username).size()) + "\n";
            response += "chat partners: " + std::to_string(chat_partners.size()) + "\n";

            for (const std::string& username : chat_partners)
                response += "  - " + username + "\n";

            response += "total stored messages: " + std::to_string(get_total_messages_count()) + "\n";

            if (!send_all(client_fd, response)) break;
        }

        else if (message == "/who")
        {
            std::vector<std::string> online_users = get_online_usernames();
            std::vector<std::string> session_users = get_session_usernames();

            size_t total_users = online_users.size() + session_users.size();

            std::string response = "\nOnline users now:\n";
            response += "total: " +std::to_string(total_users) + "\n";

            if (total_users == 0)
            {
                response += "Nobody is online.\n";
            }
            else
            {
                for (const std::string& username : online_users)
                {
                    response += " -" + username + " (tcp)\n";
                }

                for (const std::string& username : session_users)
                {
                    response += " -" + username + " (web)\n";
                }
            }
            if (!send_all(client_fd, response)) break;
        }
        
        else if (message.rfind("/chat ", 0) == 0)
        {
            std::string to_user = message.substr(6);

            if (to_user == client.username)
            {
                std::string fail = "You cannot send message to yourself.\n";
                if (!send_all(client_fd, fail)) break;
                continue;
            }

            if (to_user.empty())
            {
                std::string fail = "Usage: /chat <username>\n";
                if (!send_all(client_fd, fail)) break;
                continue;
            }

            if (!is_user_online(to_user) && !is_session_active(to_user))
            {
                std::string fail = "User is not logged in.\n";
                if (!send_all(client_fd, fail)) break;
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
            if (!send_all(client_fd, dialog_preview)) break;
            std::string enter_message = "Enter messages now. Use /back to leave this chat.\n";
            if (!send_all(client_fd, enter_message)) break;
        }

        else if (message == "/sent_messages")
        {
            std::vector<Message> sent_messages = get_sent_messages_for_user(client.username);

            if (sent_messages.empty())
            {
                std::string no_messages = "No messages sent.\n";
                if (!send_all(client_fd, no_messages)) break;
            }

            else
            {
                std::string response = "\nYour sent messages:\n";

                for (const Message& msg : sent_messages)
                {
                    response += "To [" + msg.to + "]: " + msg.text + "\n";
                }

                if (!send_all(client_fd, response)) break;
            }
            
        }
        
        else if (message == "/session_messages")
        {
            std::vector<Message> user_messages = get_messages_for_user(client.username);

            if (user_messages.empty())
            {
                std::string no_messages = "No messages.\n";
                if (!send_all(client_fd, no_messages)) break;
            }

            else
            {
                std::string response = "Your messages:\n";

                for (const Message& msg : user_messages)
                {
                    response += "From [" + msg.from + "]: " + msg.text + "\n"; 
                }

                if (!send_all(client_fd, response)) break;
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
                if (!send_all(client_fd, fail)) break;
                active_chat_user.clear();
                continue;
            }

            std::string private_msg =
                "\nNew message from [" + client.username + "]: " + message + "\n";

            add_message(client.username, active_chat_user, message);

            if (is_user_online(active_chat_user))
                send_to_user(active_chat_user, private_msg);

            std::string local_echo = "[" + client.username + "]: " + message + "\n";
            if (!send_all(client_fd, local_echo)) break;
        }

        else
        {
            std::string response = "\nInvalid choice. Please try again.\n";
            if (!send_all(client_fd, response)) break;
        }
    }

    remove_client(client_fd);
    close(client_fd);
}
