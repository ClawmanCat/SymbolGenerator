#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/logger.hpp>

#include <vector>
#include <string_view>
#include <sstream>


namespace symgen {
    extern std::string demangle_symbol(const std::string& symbol);


    inline std::vector<std::string_view> split(std::string_view sv, std::string_view sep) {
        std::vector<std::string_view> result;
        std::size_t pos = 0;

        do {
            pos = sv.find(sep);
            result.emplace_back(sv.substr(0, pos));
            if (pos != std::string_view::npos) sv.remove_prefix(pos + sep.length());
        } while (pos != std::string_view::npos);

        return result;
    }


    inline std::vector<fs::path> find_all_of_type(const fs::path& dir, std::string_view extension) {
        std::vector<fs::path> result;

        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) result.emplace_back(entry.path());
        }

        return result;
    }


    template <typename T> inline auto construct(void) {
        return [] (auto&& arg) -> T { return T { std::forward<decltype(arg)>(arg) }; };
    }
}