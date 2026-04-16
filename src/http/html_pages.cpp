#include "http/html_pages.hpp"
#include "http/http_builder.hpp"
#include "http/http_parser.hpp"
#include "chat/client_manager.hpp"
#include "storage/message_store.hpp"
#include "storage/session_store.hpp"
#include "storage/message.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <vector>

namespace
{
    std::string join_user_list_html(const std::vector<std::string>& users)
    {
        if (users.empty())
            return "<li class=\"list-entry\"><strong>none</strong><span>No active users here yet.</span></li>";

        std::string html;

        for (const std::string& username : users)
        {
            html += "<li class=\"list-entry\">";
            html += "<strong>" + escape_html(username) + "</strong>";
            html += "<span>available now</span>";
            html += "</li>";
        }

        return html;
    }

    std::string join_user_transport_list_html(const std::vector<std::string>& users,
                                              const std::string& transport_class,
                                              const std::string& transport_label)
    {
        if (users.empty())
            return "";

        std::string html;

        for (const std::string& username : users)
        {
            html += "<li class=\"list-entry\">";
            html += "<strong>" + escape_html(username) + "</strong>";
            html += "<span class=\"status-chip " + transport_class + "\">" + transport_label + "</span>";
            html += "</li>";
        }

        return html;
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

    int get_total_online_users()
    {
        return static_cast<int>(get_online_usernames().size() + get_session_usernames().size());
    }
}

std::string make_login_page(const std::string& message)
{
    std::string body;
    body += "<main class=\"shell shell-narrow page-stack\">";
    body += "<section class=\"hero\">";
    body += "<span class=\"hero-eyebrow\">Terminal + Web Chat</span>";
    body += "<h1>Mini Messenger that feels alive.</h1>";
    body += "<p>Login with a simple username and jump between browser routes, live dialogs and server inspect without losing the handmade feel of your C++ app.</p>";
    body += "<div class=\"pill-nav\">";
    body += "<a href=\"/hello\">Hello</a>";
    body += "<a href=\"/about\">About</a>";
    body += "</div>";
    body += "</section>";

    body += "<section class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Fast entry</span>";
    body += "<h2>Sign in</h2>";
    body += "<p class=\"card-copy\">No passwords, no clutter. Just pick your username and open the messenger.</p>";

    if (!message.empty())
        body += message;

    body += "<form class=\"stack-form\" method=\"POST\" action=\"/login\">";
    body += "<input class=\"text-input\" name=\"login\" placeholder=\"username\" autocomplete=\"username\">";
    body += "<button class=\"primary-button\" type=\"submit\">Enter messenger</button>";
    body += "</form>";

    body += "<div class=\"stats-row\">";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">HTTP</div><div class=\"stat-value\">8080</div><div class=\"stat-note\">Browser routes and UI.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">TCP</div><div class=\"stat-value\">8081</div><div class=\"stat-note\">Terminal chat and commands.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Now online</div><div class=\"stat-value\">" + std::to_string(get_total_online_users()) + "</div><div class=\"stat-note\">Across browser and terminal.</div></div>";
    body += "</div>";
    body += "</section></main>";
    return make_page_shell("Login", body, "login-page");
}

std::string make_chat_list_page(const std::string& current_user,
                                const std::string& message)
{
    std::vector<std::string> available_users = get_available_chat_users(current_user);
    std::vector<std::string> chat_partners = get_chat_partners_for_user(current_user);

    std::string body;
    body += "<main class=\"shell page-stack\">";
    body += "<section class=\"hero\">";
    body += "<span class=\"hero-eyebrow\">Chat hub</span>";
    body += "<h1>Pick a user and start talking.</h1>";
    body += "<p>You are logged in as <b>" + escape_html(current_user) + "</b>. Open an existing dialog or jump straight into a new one from the online list.</p>";
    body += "<div class=\"pill-nav\">";
    body += "<a href=\"/menu\">Menu</a>";
    body += "<a href=\"/inspect\">Inspect</a>";
    body += "<a href=\"/logout\">Logout</a>";
    body += "</div>";
    body += "</section>";

    if (!message.empty())
        body += "<p class=\"error-banner\">" + escape_html(message) + "</p>";

    body += "<section class=\"grid-two\">";
    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Open direct chat</span>";
    body += "<h2>Start with username</h2>";
    body += "<p class=\"card-copy\">Type a username exactly as it was used on login. If the user is online in terminal or browser, the dialog opens instantly.</p>";
    body += "<form class=\"inline-form\" method=\"POST\" action=\"/chat\">";
    body += "<input class=\"text-input\" name=\"user\" placeholder=\"username\">";
    body += "<button class=\"primary-button\" type=\"submit\">Open chat</button>";
    body += "</form>";
    body += "</div>";

    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Live state</span>";
    body += "<h2>What is happening now</h2>";
    body += "<div class=\"stats-row\">";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Online users</div><div class=\"stat-value\">" + std::to_string(available_users.size()) + "</div><div class=\"stat-note\">Ready for a new dialog.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Your chats</div><div class=\"stat-value\">" + std::to_string(chat_partners.size()) + "</div><div class=\"stat-note\">Saved chat partners.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Messages</div><div class=\"stat-value\">" + std::to_string(get_messages_for_user(current_user).size() + get_sent_messages_for_user(current_user).size()) + "</div><div class=\"stat-note\">Sent and received total.</div></div>";
    body += "</div>";
    body += "</div>";
    body += "</section>";

    body += "<section class=\"grid-two\">";
    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Online now</span>";
    body += "<h2>Available users</h2>";
    if (available_users.empty())
    {
        body += "<p class=\"muted\">No users online right now.</p>";
    }
    else
    {
        for (const std::string& username : available_users)
        {
            body += "<a class=\"user-link\" href=\"/chat/" + escape_html(username) + "\">";
            body += "<span class=\"user-link-main\">" + escape_html(username) + "</span>";
            body += "<span class=\"user-link-meta\">Open live dialog from the browser.</span>";
            body += "</a>";
        }
    }
    body += "</div>";

    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Recent people</span>";
    body += "<h2>Your chats</h2>";
    if (chat_partners.empty())
    {
        body += "<p class=\"muted\">No chats yet. Start with anyone from the list on the left.</p>";
    }
    else
    {
        for (const std::string& username : chat_partners)
        {
            body += "<a class=\"user-link\" href=\"/chat/" + escape_html(username) + "\">";
            body += "<span class=\"user-link-main\">" + escape_html(username) + "</span>";

            if (is_user_logged_in_anywhere(username))
                body += "<span class=\"user-link-meta\">Still online and reachable now.</span>";
            else
                body += "<span class=\"user-link-meta offline\">Offline, but your dialog history is saved.</span>";

            body += "</a>";
        }
    }
    body += "</div>";
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
        body += "<div class=\"message-text\">" + escape_html(msg.text) + "</div>";
        body += "</div></div>";
    }

    return body;
}

std::string make_chat_dialog_page(const std::string& current_user,
                                  const std::string& other_user)
{
    std::string body;
    body += "<main class=\"chat-shell page-stack\">";
    body += "<section class=\"hero\">";
    body += "<span class=\"hero-eyebrow\">Live dialog</span>";
    body += "<h1>Chat with " + escape_html(other_user) + "</h1>";
    body += "<p>You are logged in as <b>" + escape_html(current_user) + "</b>. Messages refresh automatically, so this page behaves more like a small app than a static form.</p>";
    body += "<div class=\"pill-nav\">";
    body += "<a href=\"/chat\">All chats</a>";
    body += "<a href=\"/menu\">Menu</a>";
    body += "</div>";
    body += "</section>";

    body += "<section class=\"chat-layout\">";
    body += "<div class=\"chat-side\">";
    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Conversation</span>";
    body += "<h2>" + escape_html(other_user) + "</h2>";
    body += "<div class=\"meta-list\">";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">Logged in as</span><span class=\"meta-value\">" + escape_html(current_user) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">Other user</span><span class=\"meta-value\">" + escape_html(other_user) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">Dialog route</span><span class=\"meta-value\">/chat/" + escape_html(other_user) + "</span></div>";
    body += "</div>";
    body += "</div>";

    body += "<div class=\"card\">";
    body += "<span class=\"eyebrow-tag\">Tips</span>";
    body += "<h2>Keep it smooth</h2>";
    body += "<p class=\"card-copy\">Messages update once per second. If the other user is in the terminal, they still receive new messages immediately.</p>";
    body += "<a class=\"text-link\" href=\"/inspect\">Open inspect</a>";
    body += "</div>";
    body += "</div>";

    body += "<div class=\"chat-main\">";
    body += "<div class=\"chat-panel\">";
    body += "<div id=\"messages\"><div class=\"empty\">Loading messages...</div></div>";
    body += "<form class=\"chat-form\" method=\"POST\" action=\"/chat/" + escape_html(other_user) + "\">";
    body += "<input id=\"messageInput\" name=\"message\" placeholder=\"Write a message\">";
    body += "<button type=\"submit\">Send message</button>";
    body += "</form>";
    body += "<div class=\"chat-links\">";
    body += "<a href=\"/chat\">Choose another user</a>";
    body += "<a href=\"/menu\">Back to menu</a>";
    body += "</div>";
    body += "</div>";
    body += "</div>";
    body += "</section></main>";
    return make_page_shell(
        "Chat with " + other_user,
        body,
        "chat-page",
        "data-chat-url=\"/chat-data/" + escape_html(other_user) + "\"",
        "/assets/js/chat.js"
    );
}

std::string make_http_inspect_page(int client_fd,
                                   const std::string& request,
                                   const std::string& request_line,
                                   const std::string& path,
                                   const std::string& current_user)
{
    sockaddr_in peer_addr{};
    sockaddr_in local_addr{};
    socklen_t peer_len = sizeof(peer_addr);
    socklen_t local_len = sizeof(local_addr);
    char peer_ip[INET_ADDRSTRLEN] = "unknown";
    char local_ip[INET_ADDRSTRLEN] = "unknown";

    if (getpeername(client_fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len) == 0)
        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, INET_ADDRSTRLEN);

    if (getsockname(client_fd, reinterpret_cast<sockaddr*>(&local_addr), &local_len) == 0)
        inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);

    std::vector<std::pair<std::string, std::string>> headers = get_headers(request);
    std::vector<std::string> online_users = get_online_usernames();
    std::vector<std::string> session_users = get_session_usernames();
    std::vector<std::string> chat_partners;

    if (!current_user.empty())
        chat_partners = get_chat_partners_for_user(current_user);

    std::string body;
    body += "<main class=\"shell simple-page page-stack\">";
    body += "<section class=\"hero\">";
    body += "<span class=\"hero-eyebrow\">Server inspect</span>";
    body += "<h1>Everything this request is carrying.</h1>";
    body += "<p>Socket details, parsed headers, request body, sessions and chat state, all in one control panel for your messenger.</p>";
    body += "<div class=\"pill-nav\">";
    body += "<a href=\"/menu\">Menu</a>";
    body += "<a href=\"/chat\">Chats</a>";
    body += "</div>";
    body += "</section>";

    body += "<section class=\"stats-row\">";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">TCP users</div><div class=\"stat-value\">" + std::to_string(online_users.size()) + "</div><div class=\"stat-note\">Connected through terminal.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Web sessions</div><div class=\"stat-value\">" + std::to_string(session_users.size()) + "</div><div class=\"stat-note\">Currently active in browser.</div></div>";
    body += "<div class=\"stat-card\"><div class=\"stat-label\">Stored messages</div><div class=\"stat-value\">" + std::to_string(get_total_messages_count()) + "</div><div class=\"stat-note\">Saved in server memory.</div></div>";
    body += "</section>";

    body += "<section class=\"inspect-grid\">";
    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Request line</span><h2>Route snapshot</h2>";
    body += "<div class=\"meta-list\">";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">method</span><span class=\"meta-value\">" + escape_html(get_method(request_line)) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">path</span><span class=\"meta-value\">" + escape_html(path) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">raw</span><span class=\"meta-value\"><code>" + escape_html(request_line) + "</code></span></div>";
    body += "</div></div>";

    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Socket info</span><h2>Connection details</h2>";
    body += "<div class=\"meta-list\">";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">peer ip</span><span class=\"meta-value\">" + escape_html(peer_ip) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">peer port</span><span class=\"meta-value\">" + std::to_string(ntohs(peer_addr.sin_port)) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">local ip</span><span class=\"meta-value\">" + escape_html(local_ip) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">local port</span><span class=\"meta-value\">" + std::to_string(ntohs(local_addr.sin_port)) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">current user</span><span class=\"meta-value\">" + escape_html(current_user.empty() ? "(guest)" : current_user) + "</span></div>";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">session cookie</span><span class=\"meta-value\">" + escape_html(get_cookie_value(request, "session_id")) + "</span></div>";
    body += "</div></div>";
    body += "</section>";

    body += "<section class=\"grid-two\">";
    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Headers</span><h2>Parsed headers</h2><ul>";
    if (headers.empty())
    {
        body += "<li class=\"list-entry\"><strong>none</strong><span>No headers parsed.</span></li>";
    }
    else
    {
        for (const auto& header : headers)
        {
            body += "<li class=\"list-entry\">";
            body += "<strong>" + escape_html(header.first) + "</strong>";
            body += "<span>" + escape_html(header.second) + "</span>";
            body += "</li>";
        }
    }
    body += "</ul></div>";

    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Body</span><h2>Request body</h2>";
    std::string raw_body = get_body(request);
    if (raw_body.empty())
        body += "<p class=\"muted\">Request body is empty.</p>";
    else
        body += "<pre>" + escape_html(raw_body) + "</pre>";
    body += "</div>";
    body += "</section>";

    body += "<section class=\"grid-two\">";
    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Server state</span><h2>Online users</h2><ul>";
    body += join_user_transport_list_html(online_users, "tcp", "tcp");
    body += join_user_transport_list_html(session_users, "web", "web");
    if (online_users.empty() && session_users.empty())
        body += "<li class=\"list-entry\"><strong>nobody</strong><span>No users online now.</span></li>";
    body += "</ul></div>";

    body += "<div class=\"card\"><span class=\"eyebrow-tag\">Your state</span><h2>Personal counters</h2>";
    body += "<div class=\"meta-list\">";
    body += "<div class=\"meta-row\"><span class=\"meta-label\">total stored messages</span><span class=\"meta-value\">" + std::to_string(get_total_messages_count()) + "</span></div>";
    if (!current_user.empty())
    {
        body += "<div class=\"meta-row\"><span class=\"meta-label\">incoming messages</span><span class=\"meta-value\">" + std::to_string(get_messages_for_user(current_user).size()) + "</span></div>";
        body += "<div class=\"meta-row\"><span class=\"meta-label\">sent messages</span><span class=\"meta-value\">" + std::to_string(get_sent_messages_for_user(current_user).size()) + "</span></div>";
        body += "<div class=\"meta-row\"><span class=\"meta-label\">chat partners</span><span class=\"meta-value\">" + std::to_string(chat_partners.size()) + "</span></div>";
    }
    else
    {
        body += "<div class=\"meta-row\"><span class=\"meta-label\">current user</span><span class=\"meta-value\">guest</span></div>";
    }
    body += "</div>";

    if (!current_user.empty())
    {
        body += "<h3>Chat partners</h3><ul>";
        body += join_user_list_html(chat_partners);
        body += "</ul>";
    }

    body += "</div>";
    body += "</section>";

    body += "<section class=\"card\"><span class=\"eyebrow-tag\">Raw request</span><h2>Unmodified request text</h2><pre>" + escape_html(request) + "</pre></section>";
    body += "<a class=\"text-link\" href=\"/menu\">Back to menu</a>";
    body += "</main>";

    return make_page_shell("Server inspect", body, "inspect-page");
}
