#pragma once

#include <iostream>
#include <mutex>
#include <syncstream>


namespace symgen {
    class logger {
    public:
        enum logger_level { VERBOSE, NORMAL, WARNING, ERROR };


        static logger& instance(void) {
            static logger i { };
            return i;
        }


        logger fork(std::string new_prefix) {
            logger copy = *this;
            copy.prefix = std::move(new_prefix);
            return copy;
        }


        void assert_that(bool cond, const auto&... msg) {
            if (!cond) [[unlikely]] {
                message(ERROR, msg...);
                std::exit(-1);
            }
        }


        void message(logger_level level, const auto&... msg) {
            if (level < this->level) return;

            std::osyncstream stream { std::cout };
            if (!prefix.empty()) stream << "[" << prefix << "] ";
            (stream << ... << msg) << "\n";

            #ifndef NDEBUG
                stream << std::flush;
            #endif
        }

        void verbose(const auto&... msg) { message(logger_level::VERBOSE, msg...); }
        void normal (const auto&... msg) { message(logger_level::NORMAL,  msg...); }
        void warning(const auto&... msg) { message(logger_level::WARNING, msg...); }
        void error  (const auto&... msg) { message(logger_level::ERROR,   msg...); }


        void set_level(logger_level level) { this->level = level; }
    private:
        std::string prefix = "SymbolGenerator";
        logger_level level = NORMAL;
    };
}