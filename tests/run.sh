#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -I"$project_root/src" \
    "$project_root/src/http/http_parser.cpp" "$project_root/tests/http_parser_test.cpp" \
    -o "$build_dir/http_parser_test"
"$build_dir/http_parser_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/http/http_io.cpp" "$project_root/tests/http_io_test.cpp" \
    -o "$build_dir/http_io_test"
"$build_dir/http_io_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/shared/utils.cpp" "$project_root/tests/chat_line_reader_test.cpp" \
    -o "$build_dir/chat_line_reader_test"
"$build_dir/chat_line_reader_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/shared/utils.cpp" "$project_root/src/chat/client_manager.cpp" \
    "$project_root/tests/chat_client_manager_test.cpp" \
    -o "$build_dir/chat_client_manager_test"
"$build_dir/chat_client_manager_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/shared/utils.cpp" "$project_root/src/chat/authorizate.cpp" \
    "$project_root/tests/chat_authorize_test.cpp" -o "$build_dir/chat_authorize_test"
"$build_dir/chat_authorize_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/shared/utils.cpp" "$project_root/src/chat/authorizate.cpp" \
    "$project_root/src/chat/client_manager.cpp" "$project_root/src/chat/chat_server.cpp" \
    "$project_root/src/storage/message_store.cpp" "$project_root/src/storage/session_store.cpp" \
    "$project_root/tests/chat_echo_test.cpp" -o "$build_dir/chat_echo_test"
"$build_dir/chat_echo_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/storage/session_store.cpp" "$project_root/tests/session_store_test.cpp" \
    -o "$build_dir/session_store_test"
"$build_dir/session_store_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/storage/message_store.cpp" "$project_root/tests/message_store_test.cpp" \
    -o "$build_dir/message_store_test"
"$build_dir/message_store_test"

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pthread -I"$project_root/src" \
    "$project_root/src/shared/utils.cpp" "$project_root/src/chat/client_manager.cpp" \
    "$project_root/src/storage/message_store.cpp" "$project_root/src/storage/session_store.cpp" \
    "$project_root/src/http/http_server.cpp" "$project_root/src/http/http_parser.cpp" \
    "$project_root/src/http/http_builder.cpp" "$project_root/src/http/html_pages.cpp" \
    "$project_root/src/http/http_io.cpp" "$project_root/tests/http_session_test.cpp" \
    -o "$build_dir/http_session_test"
"$build_dir/http_session_test"
