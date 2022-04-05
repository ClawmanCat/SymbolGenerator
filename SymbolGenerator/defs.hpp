#pragma once

#include <SymbolGenerator/legacy.hpp>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <range/v3/all.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>


namespace symgen {
    namespace fs    = std::filesystem;
    namespace views = ranges::views;
    using namespace std::string_literals;
    using namespace std::string_view_literals;


    template <typename K> using default_hash = typename absl::flat_hash_set<K>::hasher;
    template <typename K> using default_eq   = typename absl::flat_hash_set<K>::key_equal;


    template <typename K, typename V, typename Hash = default_hash<K>, typename Eq = default_eq<K>>
    using hash_map = absl::flat_hash_map<K, V, Hash, Eq>;

    template <typename K, typename Hash = default_hash<K>, typename Eq = default_eq<K>>
    using hash_set = absl::flat_hash_set<K, Hash, Eq>;
}