#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -I"$project_root/src" \
    "$project_root/src/http/http_parser.cpp" "$project_root/tests/http_parser_test.cpp" \
    -o "$build_dir/http_parser_test"
"$build_dir/http_parser_test"
