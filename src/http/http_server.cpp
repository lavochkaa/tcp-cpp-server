#include "http/http_server.hpp"

#include "chat/client_manager.hpp"
#include "storage/message_store.hpp"
#include "storage/session_store.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace
{
    const char* kNotFoundPage = "<html><body><h1>404 Not Found</h1></body></html>";

    std::string escape_html(const std::string& text);

    std::string make_response(const std::string& status_line,
                              const std::string& content_type,
                              const std::string& body)
    {
        return status_line + "\r\n"
               "Content-Type: " + content_type + "\r\n"
               "Content-Length: " + std::to_string(body.size()) + "\r\n"
               "Connection: close\r\n"
               "\r\n" +
               body;
    }

    std::string make_page_shell(const std::string& title,
                                const std::string& body_content,
                                const std::string& body_class = "",
                                const std::string& body_attrs = "",
                                const std::string& script_src = "")
    {
        std::string html = "<!DOCTYPE html><html><head>";
        html += "<meta charset=\"utf-8\">";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += "<title>" + escape_html(title) + "</title>";
        html += "<link rel=\"stylesheet\" href=\"/assets/css/app.css\">";
        html += "</head><body";

        if (!body_class.empty())
            html += " class=\"" + body_class + "\"";

        if (!body_attrs.empty())
            html += " " + body_attrs;

        html += ">";
        html += body_content;

        if (!script_src.empty())
            html += "<script src=\"" + script_src + "\"></script>";

        html += "</body></html>";
        return html;
    }

    std::string read_file(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string escape_html(const std::string& text)
    {
        std::string escaped;

        for (char ch : text)
        {
            switch (ch)
            {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                default: escaped += ch; break;
            }
        }

        return escaped;
    }

    void append_unique_users(std::vector<std::string>& target,
                             const std::vector<std::string>& source,
                             const std::string& current_user)
    {
        for (const std::string& username : source)
        {
            if (username.empty() || username == current_user)
                continue;

            bool exists = false;

            for (const std::string& saved : target)
            {
                if (saved == username)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
                target.push_back(username);
        }
    }

    std::vector<std::string> get_available_chat_users(const std::string& current_user)
    {
        std::vector<std::string> users;

        append_unique_users(users, get_online_usernames(), current_user);
        append_unique_users(users, get_session_usernames(), current_user);

        return users;
    }

    bool is_user_logged_in_anywhere(const std::string& username)
    {
        return is_user_online(username) || is_session_active(username);
    }

    std::string make_login_page(const std::string& message)
    {
        std::string body;
        body += "<main class=\"shell shell-narrow\">";
        body += "<section class=\"card login-card\">";
        body += "<h1>Mini Messenger</h1>";
        body += "<p class=\"lead\">Login with your username to enter chat.</p>";
        body += message;
        body += "<form class=\"stack-form\" method=\"POST\" action=\"/login\">";
        body += "<input class=\"text-input\" name=\"login\" placeholder=\"username\" autocomplete=\"username\">";
        body += "<button class=\"primary-button\" type=\"submit\">Sign in</button>";
        body += "</form>";
        body += "</section></main>";
        return make_page_shell("Login", body, "login-page");
    }

    std::string make_html_response(const std::string& body)
    {
        return make_response("HTTP/1.1 200 OK", "text/html; charset=utf-8", body);
    }

    std::string make_404_response(const std::string& body)
    {
        return make_response("HTTP/1.1 404 Not Found", "text/html; charset=utf-8", body);
    }

    std::string make_redirect_response(const std::string& location, const std::string& cookie_header = "")
    {
        std::string response = "HTTP/1.1 302 Found\r\n";
        response += "Location: " + location + "\r\n";

        if (!cookie_header.empty())
            response += cookie_header + "\r\n";

        response += "Connection: close\r\n";
        response += "\r\n";

        return response;
    }

    std::string load_404_response()
    {
        std::string page404 = read_file("web/pages/404.html");
        if (page404.empty())
            return make_404_response(kNotFoundPage);

        return make_404_response(page404);
    }

    std::string load_html_page_response(const std::string& path)
    {
        std::string body = read_file(path);
        if (body.empty())
            return load_404_response();

        return make_html_response(body);
    }

    std::string load_asset_response(const std::string& path, const std::string& content_type)
    {
        std::string body = read_file(path);
        if (body.empty())
            return load_404_response();

        return make_response("HTTP/1.1 200 OK", content_type, body);
    }

    std::string get_request_line(const std::string& request)
    {
        size_t pos = request.find("\r\n");
        if (pos == std::string::npos)
            return request;

        return request.substr(0, pos);
    }

    std::string get_body(const std::string& request)
    {
        size_t pos = request.find("\r\n\r\n");
        if (pos == std::string::npos)
            return "";

        return request.substr(pos + 4);
    }

    std::string get_cookie_value(const std::string& request, const std::string& key)
    {
        const std::string cookie_line = "Cookie: ";
        size_t start = request.find(cookie_line);

        if (start == std::string::npos)
            return "";

        size_t end = request.find("\r\n", start);
        std::string cookies = request.substr(start + cookie_line.size(), end - start - cookie_line.size());

        std::string prefix = key + "=";
        size_t value_start = cookies.find(prefix);

        if (value_start == std::string::npos)
            return "";

        value_start += prefix.size();

        size_t value_end = cookies.find(";", value_start);
        if (value_end == std::string::npos)
            value_end = cookies.size();

        return cookies.substr(value_start, value_end - value_start);
    }

    std::string get_current_http_user(const std::string& request)
    {
        std::string session_id = get_cookie_value(request, "session_id");
        if (session_id.empty())
            return "";

        return get_username_by_session(session_id);
    }

    std::string get_path(const std::string& request_line)
    {
        size_t first_space = request_line.find(' ');
        if (first_space == std::string::npos)
            return "";

        size_t second_space = request_line.find(' ', first_space + 1);
        if (second_space == std::string::npos)
            return "";

        return request_line.substr(first_space + 1, second_space - first_space - 1);
    }

    std::string get_form_value(const std::string& body, const std::string& key)
    {
        std::string prefix = key + "=";
        size_t start = body.find(prefix);

        if (start == std::string::npos)
            return "";

        start += prefix.size();

        size_t end = body.find('&', start);
        if (end == std::string::npos)
            end = body.size();

        return body.substr(start, end - start);
    }

    std::string make_chat_list_page(const std::string& current_user, const std::string& message = "")
    {
        std::vector<std::string> available_users = get_available_chat_users(current_user);
        std::vector<std::string> chat_partners = get_chat_partners_for_user(current_user);

        std::string body;
        body += "<main class=\"shell\">";
        body += "<section class=\"page-head\">";
        body += "<h1>Chats</h1>";
        body += "<p class=\"lead\">You are logged in as <b>" + escape_html(current_user) + "</b></p>";
        body += "</section>";

        if (!message.empty())
            body += "<p class=\"error-banner\">" + escape_html(message) + "</p>";

        body += "<section class=\"card\">";
        body += "<h2>Start chat</h2>";
        body += "<form class=\"inline-form\" method=\"POST\" action=\"/chat\">";
        body += "<input class=\"text-input\" name=\"user\" placeholder=\"username\">";
        body += "<button class=\"primary-button\" type=\"submit\">Open chat</button>";
        body += "</form>";
        body += "</section>";

        body += "<section class=\"card\">";
        body += "<h2>Users online now</h2>";
        if (available_users.empty())
        {
            body += "<p class=\"muted\">No users online.</p>";
        }
        else
        {
            for (const std::string& username : available_users)
            {
                body += "<a class=\"user-link\" href=\"/chat/" + escape_html(username) + "\">";
                body += escape_html(username);
                body += "</a>";
            }
        }
        body += "</section>";

        body += "<section class=\"card\">";
        body += "<h2>Your chats</h2>";
        if (chat_partners.empty())
        {
            body += "<p class=\"muted\">No chats yet.</p>";
        }
        else
        {
            for (const std::string& username : chat_partners)
            {
                body += "<a class=\"user-link\" href=\"/chat/" + escape_html(username) + "\">";
                body += escape_html(username);

                if (!is_user_logged_in_anywhere(username))
                    body += "<span class=\"offline\"> (offline)</span>";

                body += "</a>";
            }
        }
        body += "</section>";

        body += "<a class=\"text-link\" href=\"/menu\">Back to menu</a>";
        body += "</main>";
        return make_page_shell("Chats", body, "chat-list-page");
    }

    std::string make_chat_messages_html(const std::string& current_user,
                                        const std::vector<Message>& dialog)
    {
        if (dialog.empty())
            return "<div class=\"empty-state\">No messages yet.</div>";

        std::string body;

        for (const Message& msg : dialog)
        {
            bool mine = (msg.from == current_user);

            body += "<div class=\"message-row";
            if (mine)
                body += " mine";
            body += "\">";

            body += "<div class=\"message-bubble";
            if (mine)
                body += " mine";
            body += "\">";
            body += "<div class=\"message-author\">" + escape_html(msg.from) + "</div>";
            body += "<div>" + escape_html(msg.text) + "</div>";
            body += "</div></div>";
        }

        return body;
    }

    std::string make_chat_dialog_page(const std::string& current_user,
                                      const std::string& other_user)
    {
        std::string body;
        body += "<div class=\"chat-shell\">";
        body += "<div class=\"chat-head\">";
        body += "<h1>Chat with " + escape_html(other_user) + "</h1>";
        body += "<p>You are logged in as <b>" + escape_html(current_user) + "</b></p>";
        body += "</div>";
        body += "<div id=\"messages\"><div class=\"empty\">Loading messages...</div></div>";
        body += "<form class=\"chat-form\" method=\"POST\" action=\"/chat/" + escape_html(other_user) + "\">";
        body += "<input id=\"messageInput\" name=\"message\" placeholder=\"Write a message\">";
        body += "<button type=\"submit\">Send</button>";
        body += "</form>";
        body += "<div class=\"chat-links\">";
        body += "<a href=\"/chat\">Choose another user</a>";
        body += "<a href=\"/menu\">Back to menu</a>";
        body += "</div></div>";
        return make_page_shell(
            "Chat with " + other_user,
            body,
            "chat-page",
            "data-chat-url=\"/chat-data/" + escape_html(other_user) + "\"",
            "/assets/js/chat.js"
        );
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
        else if (request_line.rfind("POST ", 0) == 0)
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
        {
            response = make_redirect_response("/");
        }
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
        else if (request_line.rfind("POST ", 0) == 0)
        {
            std::string body = get_body(request);
            std::string text = get_form_value(body, "message");

            if (text.empty())
            {
                response = make_redirect_response(path);
            }
            else
            {
                add_message(current_user, other_user, text);

                if (is_user_online(other_user))
                {
                    std::string private_msg =
                        "\nNew message from [" + current_user + "]: " + text + "\n";
                    send_to_user(other_user, private_msg);
                }

                response = make_redirect_response(path);
            }
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
