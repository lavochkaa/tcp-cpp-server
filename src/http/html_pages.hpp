#pragma once

#include "storage/message.hpp"

#include <string>
#include <vector>

std::string make_login_page(const std::string& message);

std::string make_chat_list_page(const std::string& current_user,
                                const std::string& message = "");

std::string make_chat_dialog_page(const std::string& current_user,
                                  const std::string& other_user);

std::string make_chat_messages_html(const std::string& current_user,
                                    const std::vector<Message>& dialog);

std::string make_http_inspect_page(int client_fd,
                                   const std::string& request,
                                   const std::string& request_line,
                                   const std::string& path,
                                   const std::string& current_user);
