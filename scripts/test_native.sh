#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
sources=(src/MorseTable.cpp src/Settings.cpp src/MorseTransmitter.cpp)
for optional in src/Configuration.cpp src/ConfigApi.cpp; do
  if [[ -f "$optional" ]]; then sources+=("$optional"); fi
done
for test_file in tests/native/test_*.cpp; do
  test_name=$(basename "$test_file" .cpp)
  "${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie \
    -Iinclude "${sources[@]}" "$test_file" -o "$build_dir/$test_name"
  "$build_dir/$test_name"
done
