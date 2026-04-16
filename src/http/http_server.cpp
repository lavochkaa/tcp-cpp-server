#include "http/http_server.hpp"
#include "http/http_parser.hpp"
#include "http/http_builder.hpp"
#include "http/html_pages.hpp"

#include "chat/client_manager.hpp"
#include "storage/message_store.hpp"
#include "storage/session_store.hpp"

#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
    std::string get_current_http_user(const std::string& request)
    {
        std::string session_id = get_cookie_value(request, "session_id");
        if (session_id.empty())
            return "";

        return get_username_by_session(session_id);
    }

    bool is_user_logged_in_anywhere(const std::string& username)
    {
        return is_user_online(username) || is_session_active(username);
    }
}

void handle_http_client(int client_fd)
{
    char buffer[4096] = {0};

    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        close(client_fd);
        return;
    }

    std::string request(buffer, bytes);
    std::string request_line = get_request_line(request);
    std::string path = get_path(request_line);
    std::string method = get_method(request_line);

    std::string response;

    if (path == "/")
    {
        response = make_html_response(make_login_page(""));
    }
    else if (path == "/assets/css/app.css")
    {
        response = load_asset_response("web/assets/css/app.css", "text/css; charset=utf-8");
    }
    else if (path == "/assets/js/chat.js")
    {
        response = load_asset_response("web/assets/js/chat.js", "application/javascript; charset=utf-8");
    }
    else if (path == "/login")
    {
        std::string body = get_body(request);
        std::string login = get_form_value(body, "login");

        if (login.empty())
        {
            response = make_html_response(
                make_login_page("<p style=\"color: red;\">Login cannot be empty.</p>")
            );
        }
        else if (is_user_logged_in_anywhere(login))
        {
            response = make_html_response(
                make_login_page("<p style=\"color: red;\">User already logged in.</p>")
            );
        }
        else
        {
            std::string session_id = create_session(login);
            std::string cookie = "Set-Cookie: session_id=" + session_id;
            response = make_redirect_response("/menu", cookie);
        }
    }
    else if (path == "/hello")
    {
        response = load_html_page_response("web/pages/hello.html");
    }
    else if (path == "/about")
    {
        response = load_html_page_response("web/pages/about.html");
    }
    else if (path == "/inspect")
    {
        response = make_html_response(
            make_http_inspect_page(client_fd, request, request_line, path,
                                   get_current_http_user(request))
        );
    }
    else if (path == "/menu")
    {
        if (get_current_http_user(request).empty())
            response = make_redirect_response("/");
        else
            response = load_html_page_response("web/pages/menu.html");
    }
    else if (path == "/chat")
    {
        std::string current_user = get_current_http_user(request);

        if (current_user.empty())
        {
            response = make_redirect_response("/");
        }
        else if (method == "POST")
        {
            std::string body = get_body(request);
            std::string to_user = get_form_value(body, "user");

            if (to_user.empty())
                response = make_html_response(make_chat_list_page(current_user, "Username cannot be empty."));
            else if (to_user == current_user)
                response = make_html_response(make_chat_list_page(current_user, "You cannot start chat with yourself."));
            else if (!is_user_logged_in_anywhere(to_user))
                response = make_html_response(make_chat_list_page(current_user, "User is not logged in."));
            else
                response = make_redirect_response("/chat/" + to_user);
        }
        else
        {
            response = make_html_response(make_chat_list_page(current_user));
        }
    }
    else if (path.rfind("/chat-data/", 0) == 0)
    {
        std::string current_user = get_current_http_user(request);
        std::string other_user = path.substr(11);

        if (current_user.empty())
            response = make_redirect_response("/");
        else
        {
            std::vector<Message> dialog = get_dialog_messages(current_user, other_user);
            response = make_html_response(make_chat_messages_html(current_user, dialog));
        }
    }
    else if (path.rfind("/chat/", 0) == 0)
    {
        std::string current_user = get_current_http_user(request);
        std::string other_user = path.substr(6);

        if (current_user.empty())
        {
            response = make_redirect_response("/");
        }
        else if (other_user.empty())
        {
            response = make_redirect_response("/chat");
        }
        else if (other_user == current_user)
        {
            response = make_html_response(make_chat_list_page(current_user, "You cannot open chat with yourself."));
        }
        else if (!is_user_logged_in_anywhere(other_user))
        {
            response = make_html_response(make_chat_list_page(current_user, "User is not logged in."));
        }
        else if (method == "POST")
        {
            std::string body = get_body(request);
            std::string text = get_form_value(body, "message");

            if (!text.empty())
            {
                add_message(current_user, other_user, text);

                if (is_user_online(other_user))
                {
                    std::string private_msg =
                        "\nNew message from [" + current_user + "]: " + text + "\n";
                    send_to_user(other_user, private_msg);
                }
            }

            response = make_redirect_response(path);
        }
        else
        {
            response = make_html_response(make_chat_dialog_page(current_user, other_user));
        }
    }
    else if (path == "/logout")
    {
        std::string session_id = get_cookie_value(request, "session_id");

        if (!session_id.empty())
            remove_session(session_id);

        response = make_redirect_response("/", "Set-Cookie: session_id=; Max-Age=0");
    }
    else
    {
        response = load_404_response();
    }

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}
