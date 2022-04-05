#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/utility.hpp>
#include <SymbolGenerator/logger.hpp>

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <algorithm>


namespace symgen {
    class argument_parser {
    public:
        static argument_parser& instance(void) {
            static argument_parser i;
            return i;
        }


        struct none_t {
            friend std::ostream& operator<<(std::ostream& stream, const none_t& self) {
                stream << "[None]";
                return stream;
            }
        };

        using argument_t = std::variant<std::string, long long, bool, none_t>;


        template <typename Rng> void add_arguments(const Rng& range) {
            enum { NO_TOKEN, HAS_KEY, HAS_VAL } state = NO_TOKEN;
            std::string pending_key, pending_value;


            for (const auto& arg : range) {
                std::string_view sv { arg };
                bool has_prefix = false;

                while (sv.starts_with('-')) sv.remove_prefix(1), has_prefix = true;

                if (state == NO_TOKEN) {
                    pending_key = sv;
                    state = HAS_KEY;
                    continue;
                }

                if (state == HAS_KEY || state == HAS_VAL) {
                    if (has_prefix) {
                        arguments.emplace(std::move(pending_key), to_argument(pending_value));

                        pending_key = sv;
                        pending_value.clear();

                        state = HAS_KEY;
                    } else {
                        if (!pending_value.empty()) pending_value += " ";
                        pending_value += sv;
                        state = HAS_VAL;
                    }

                    continue;
                }
            }


            if (state != NO_TOKEN) {
                arguments.emplace(std::move(pending_key), to_argument(pending_value));
            }
        }


        bool has_argument(std::string_view key) const {
            return arguments.contains(key);
        }


        template <typename T> std::optional<T> get_argument(std::string_view key) const {
            auto it = arguments.find(key);
            if (it == arguments.end()) return std::nullopt;

            if (std::holds_alternative<T>(it->second)) {
                return std::get<T>(it->second);
            } else {
                return std::nullopt;
            }
        }


        template <typename T> void require_argument(std::string_view key) {
            logger::instance().assert_that(has_argument(key), "Missing required argument ", key);
        }


        void print_arguments(void) const {
            logger::instance().normal("Argument list of size ", arguments.size(), ":");

            for (const auto& [k, v] : arguments) {
                std::visit([&, key = &k] (const auto& v) {
                    logger::instance().normal(*key, ": ", v);
                }, v);
            }
        }
    private:
        hash_map<std::string, argument_t> arguments;


        static argument_t to_argument(std::string_view value) {
            if (value.empty()) return none_t { };

            std::string lower;
            std::transform(value.begin(), value.end(), std::back_inserter(lower), [] (char c) { return char(std::tolower(c)); });

            if (lower == "true" ) return true;
            if (lower == "false") return false;

            try {
                return argument_t { std::stoll(lower) };
            } catch (...) {}

            return argument_t { std::string { value } };
        }
    };
}