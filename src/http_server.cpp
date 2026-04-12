#include "http_server.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>
#include <sstream>

std::string read_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string make_login_page(const std::string& message)
{
    return "<html><body>"
           "<h1>Login</h1>"
           + message +
           "<form method=\"POST\" action=\"/login\">"
           "<input name=\"login\" placeholder=\"login\">"
           "<input name=\"password\" type=\"password\" placeholder=\"password\">"
           "<button type=\"submit\">Sign in</button>"
           "</form>"
           "</body></html>";
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

bool check_login_password(const std::string& login, const std::string& password)
{
    return (login == "admin" && password == "admin") ||
           (login == "user" && password == "qwerty") ||
           (login == "test" && password == "123");
}

std::string make_html_response(const std::string& body)
{
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "Connection: close\r\n"
           "\r\n" +
           body;
}

std::string make_404_response(const std::string& body)
{
    return "HTTP/1.1 404 Not Found\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "Connection: close\r\n"
           "\r\n" +
           body;
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

    std::cout << request << "\n";

    std::string response;

    if (request_line.find("GET / ") == 0)
    {
        response = make_html_response(make_login_page(""));
    }
    else if (request_line.find("POST /login ") == 0)
    {
        std::string body = get_body(request);

        std::string login = get_form_value(body, "login");
        std::string password = get_form_value(body, "password");

        bool ok = check_login_password(login, password);

        if (ok)
        {
            std::cout << "User: " << login << " logged in successfully.\n";

            std::string page = read_file("html_pages/menu.html");
            if (page.empty())
                response = make_html_response("<html><body><h1>Menu file not found</h1></body></html>");
            else
                response = make_html_response(page);
        }
        else
        {
            std::string message = "<p style=\"color: red;\">Invalid login or password.</p>";
            response = make_html_response(make_login_page(message));
        }
    }
    else if (request_line.find("GET /hello ") == 0)
    {
        std::string body = read_file("html_pages/hello.html");

        if (body.empty())
        {
            std::string page404 = read_file("html_pages/404.html");
            if (page404.empty())
                response = make_404_response("<html><body><h1>404 Not Found</h1></body></html>");
            else
                response = make_404_response(page404);
        }
        else
        {
            response = make_html_response(body);
        }
    }
    else if (request_line.find("GET /about ") == 0)
    {
        std::string body = read_file("html_pages/about.html");

        if (body.empty())
        {
            std::string page404 = read_file("html_pages/404.html");
            if (page404.empty())
                response = make_404_response("<html><body><h1>404 Not Found</h1></body></html>");
            else
                response = make_404_response(page404);
        }
        else
        {
            response = make_html_response(body);
        }
    }
    else if (request_line.find("GET /menu ") == 0)
    {
        std::string body = read_file("html_pages/menu.html");

        if (body.empty())
        {
            std::string page404 = read_file("html_pages/404.html");
            if (page404.empty())
                response = make_404_response("<html><body><h1>404 Not Found</h1></body></html>");
            else
                response = make_404_response(page404);
        }
        else
        {
            response = make_html_response(body);
        }
    }
    else
    {
        std::string page404 = read_file("html_pages/404.html");
        if (page404.empty())
            response = make_404_response("<html><body><h1>404 Not Found</h1></body></html>");
        else
            response = make_404_response(page404);
    }

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}
