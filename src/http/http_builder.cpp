#include "http/http_builder.hpp"

#include <fstream>
#include <sstream>
#include <string>

static const char* kNotFoundPage = "<html><body><h1>404 Not Found</h1></body></html>";

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
            default:  escaped += ch; break;
        }
    }

    return escaped;
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

std::string make_html_response(const std::string& body)
{
    return make_response("HTTP/1.1 200 OK", "text/html; charset=utf-8", body);
}

std::string make_404_response(const std::string& body)
{
    return make_response("HTTP/1.1 404 Not Found", "text/html; charset=utf-8", body);
}

std::string make_redirect_response(const std::string& location,
                                   const std::string& cookie_header)
{
    std::string response = "HTTP/1.1 302 Found\r\n";
    response += "Location: " + location + "\r\n";

    if (!cookie_header.empty())
        response += cookie_header + "\r\n";

    response += "Connection: close\r\n";
    response += "\r\n";

    return response;
}

std::string make_page_shell(const std::string& title,
                            const std::string& body_content,
                            const std::string& body_class,
                            const std::string& body_attrs,
                            const std::string& script_src)
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
