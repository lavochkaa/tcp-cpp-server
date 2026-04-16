#pragma once

#include <string>

std::string make_response(const std::string& status_line,
                          const std::string& content_type,
                          const std::string& body);

std::string make_html_response(const std::string& body);
std::string make_404_response(const std::string& body);
std::string make_redirect_response(const std::string& location,
                                   const std::string& cookie_header = "");

std::string make_page_shell(const std::string& title,
                            const std::string& body_content,
                            const std::string& body_class = "",
                            const std::string& body_attrs = "",
                            const std::string& script_src = "");

std::string escape_html(const std::string& text);
std::string read_file(const std::string& path);
std::string load_404_response();
std::string load_html_page_response(const std::string& path);
std::string load_asset_response(const std::string& path, const std::string& content_type);
