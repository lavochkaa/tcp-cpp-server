#include "http/http_parser.hpp"

#include <iostream>
#include <string>

namespace
{
    int failures = 0;

    void expect_equal(const std::string& name, const std::string& actual,
                      const std::string& expected)
    {
        if (actual != expected)
        {
            std::cerr << name << ": expected '" << expected << "', got '" << actual << "'\n";
            ++failures;
        }
    }
}

int main()
{
    expect_equal("query string is not part of route",
                 get_path("GET /chat?view=recent HTTP/1.1"), "/chat");
    expect_equal("encoded question mark remains in route",
                 get_path("GET /chat/bob%3Fname?view=recent HTTP/1.1"),
                 "/chat/bob%3Fname");

    const std::string request =
        "GET /menu HTTP/1.1\r\n"
        "X-Cookie: session_id=wrong\r\n"
        "Cookie: other_session_id=wrong; session_id=correct\r\n"
        "\r\n";

    expect_equal("exact cookie header and name",
                 get_cookie_value(request, "session_id"), "correct");
    expect_equal("missing cookie name",
                 get_cookie_value(request, "missing"), "");
    expect_equal("cookie text in body",
                 get_cookie_value("POST / HTTP/1.1\r\n\r\nCookie: session_id=wrong", "session_id"), "");
    expect_equal("exact form name",
                 get_form_value("other_login=wrong&login=correct", "login"), "correct");
    expect_equal("missing form name",
                 get_form_value("other_login=wrong", "login"), "");
    expect_equal("equals sign in form value",
                 get_form_value("login=first=last", "login"), "first=last");
    expect_equal("plus encoded form value",
                 get_form_value("message=hello+world", "message"), "hello world");
    expect_equal("percent encoded form value",
                 get_form_value("message=hello%20world%21", "message"), "hello world!");
    expect_equal("encoded form delimiters",
                 get_form_value("message=a%26b%3Dc", "message"), "a&b=c");
    expect_equal("invalid percent escape preserved",
                 get_form_value("message=100%zz", "message"), "100%zz");
    expect_equal("path segment special characters",
                 encode_path_segment("bob#?/+%"), "bob%23%3F%2F%2B%25");
    expect_equal("path plus stays plus",
                 decode_path_segment("bob+one"), "bob+one");
    expect_equal("encoded path segment round trip",
                 decode_path_segment("bob%23%3F%2F%2B%25"), "bob#?/+%");

    return failures == 0 ? 0 : 1;
}
