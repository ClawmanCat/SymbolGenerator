#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/argument_parser.hpp>
#include <SymbolGenerator/utility.hpp>

#include <regex>


namespace symgen {
    class rule_cache {
    public:
        static rule_cache& instance(void) {
            static rule_cache i;
            return i;
        }


        [[nodiscard]] const std::vector<std::regex>& get_includes(void) const { return include; }
        [[nodiscard]] const std::vector<std::regex>& get_excludes(void) const { return exclude; }
        [[nodiscard]] const std::vector<std::regex>& get_force_includes(void) const { return force_include; }
        [[nodiscard]] const std::vector<std::regex>& get_force_excludes(void) const { return force_exclude; }
    private:
        std::vector<std::regex> include, exclude, force_include, force_exclude;


        rule_cache(void) {
            constexpr std::array fields {
                std::tuple { &rule_cache::include, "y"sv },
                std::tuple { &rule_cache::exclude, "n"sv },
                std::tuple { &rule_cache::force_include, "yo"sv },
                std::tuple { &rule_cache::force_exclude, "no"sv }
            };


            for (const auto& [field, argument] : fields) {
                auto value = argument_parser::instance().template get_argument<std::string>(argument);
                if (!value) continue;

                std::string_view sv { *value };
                for (auto substr : split(sv, " ")) {
                    (this->*field).emplace_back(substr.begin(), substr.end());
                }
            }
        }
    };
}