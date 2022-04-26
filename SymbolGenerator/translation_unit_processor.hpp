#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/utility.hpp>
#include <SymbolGenerator/argument_parser.hpp>
#include <SymbolGenerator/rule_cache.hpp>
#include <SymbolGenerator/logger.hpp>


namespace symgen {
    enum class symbol_state : char {
        FUNCTION = 'F', DATA = 'D', EXCLUDED = 'E'
    };


    struct included_symbol {
        std::string mangled_name;
        bool is_data_symbol;

        bool operator==(const included_symbol& other) const {
            return mangled_name == other.mangled_name;
        }

        std::size_t hash(void) const {
            return hash_of(mangled_name);
        }
    };


    class translation_unit_processor {
    public:
        constexpr static std::string_view CACHE_FMT_VERSION = "0.0.2";


        void process(const fs::path& directory, std::string_view name);
        [[nodiscard]] const std::vector<included_symbol>& get_included_symbols(void) const { return included_symbols; }
    private:
        mutable logger log;

        hash_map<std::string, symbol_state> cached_symbols;
        std::vector<included_symbol> included_symbols;
        bool has_uncached_symbols = false;

        void parse(const fs::path& obj_path);
        void load_cache(const fs::path& path);
        void write_cache(const fs::path& path) const;
    };
}