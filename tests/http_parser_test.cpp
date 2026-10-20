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

    return failures == 0 ? 0 : 1;
}
