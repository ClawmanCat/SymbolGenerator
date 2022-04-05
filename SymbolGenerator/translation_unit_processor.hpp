#pragma once

#include <SymbolGenerator/defs.hpp>
#include <SymbolGenerator/argument_parser.hpp>
#include <SymbolGenerator/rule_cache.hpp>
#include <SymbolGenerator/logger.hpp>


namespace symgen {
    class translation_unit_processor {
    public:
        void process(const fs::path& directory, std::string_view name);
        [[nodiscard]] const std::vector<std::string>& get_included_symbols(void) const { return included_symbols; }
    private:
        mutable logger log;

        hash_map<std::string, bool> cached_symbols;
        std::vector<std::string> included_symbols;
        bool has_uncached_symbols = false;

        void parse(const fs::path& obj_path);
        void load_cache(const fs::path& path);
        void write_cache(const fs::path& path) const;
    };
}