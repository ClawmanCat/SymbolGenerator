#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/logger.hpp>

#include <vector>
#include <string_view>
#include <sstream>


namespace symgen {
    using filter_function = int(*)(const char*, const void*, const void*);

    extern filter_function load_filter_function(const fs::path& path);
    extern std::string demangle_symbol(const std::string& symbol);


    template <typename... Ts> inline std::string stream_to_string(const Ts&... args) {
        std::stringstream stream;
        (stream << ... << args);
        return stream.str();
    }


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


    template <typename Rng> inline std::string join(const Rng& range, std::string_view sep) {
        std::string result;
        if (std::size(range) == 0) return result;

        for (const auto& elem : range | views::drop_last(1)) {
            result += elem;
            result += sep;
        }
        
        result += *(range.begin() + std::size(range) - 1);
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


    // Checks if the two given settings dicts are compatible. Returns an error message if they're not, and nullopt otherwise.
    inline std::optional<std::string> check_settings_compatible(const hash_map<std::string, std::string>& cached, const hash_map<std::string, std::string>& current) {
        for (const auto& [setting, value] : cached) {
            if (!current.contains(setting)) {
                return stream_to_string("setting ", setting, " is set in cache but not present currently.");
            }
        }

        for (const auto& [setting, value] : current) {
            if (!cached.contains(setting)) {
                return stream_to_string("setting ", setting, " is set currently but not present in cache.");
            }
        }


        // Setting values are space-separated lists, so we have to account for the fact they may be re-ordered.
        for (const auto& [setting, cached_value] : cached) {
            const auto& current_value = current.at(setting);

            auto cached_value_components  = split(cached_value, " ");
            ranges::sort(cached_value_components);
            auto current_value_components = split(current_value, " ");
            ranges::sort(current_value_components);

            std::vector<std::string_view> difference;
            ranges::set_symmetric_difference(cached_value_components, current_value_components, std::back_inserter(difference));

            if (!difference.empty()) {
                std::string error_msg = stream_to_string(
                    "setting ", setting,
                    " has a different value in cache (", cached_value,
                    ") than its current value (", current_value, ")."
                );

                if (difference.size() > 2) {
                    std::string diff_string = join(difference, ", ");
                    error_msg += stream_to_string(" The following values are mismatched: ", diff_string, ".");
                }

                return error_msg;
            }
        }


        return std::nullopt;
    }
}