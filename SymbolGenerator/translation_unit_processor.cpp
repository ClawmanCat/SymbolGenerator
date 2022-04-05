#include <SymbolGenerator/translation_unit_processor.hpp>
#include <SymbolGenerator/rule_cache.hpp>
#include <SymbolGenerator/utility.hpp>
#include <SymbolGenerator/logger.hpp>

#include <coffi/coffi.hpp>

#include <fstream>
#include <sstream>


namespace symgen {
    void translation_unit_processor::process(const fs::path& directory, std::string_view name) {
        this->log = logger::instance().fork(std::string { name });
        log.normal("Processing translation unit ", name, ".obj");

        fs::path obj_path   = directory / (std::string { name } + ".obj");
        fs::path cache_path = directory / (std::string { name } + ".objcache");
        bool do_cache       = argument_parser::instance().has_argument("cache");

        if (do_cache) load_cache(cache_path);
        parse(obj_path);
        if (do_cache && has_uncached_symbols) write_cache(cache_path);
    }


    void translation_unit_processor::parse(const fs::path& obj_path) {
        //std::ifstream stream { obj_path };
        //log.assert_that(!stream.fail(), "Failed to read obj file ", obj_path);

        COFFI::coffi reader;
        reader.load(obj_path.string());
        log.verbose(reader.get_symbols()->size(), " symbols found.");


        const auto& args_y  = rule_cache::instance().get_includes();
        const auto& args_n  = rule_cache::instance().get_excludes();
        const auto& args_yo = rule_cache::instance().get_force_includes();
        const auto& args_no = rule_cache::instance().get_force_excludes();


        for (const auto& sym : *reader.get_symbols()) {
            std::string mangled_name = sym.get_name();

            if (auto it = cached_symbols.find(mangled_name); it != cached_symbols.end()) {
                if (it->second) included_symbols.push_back(std::move(mangled_name));
                continue;
            }


            std::string demangled_name  = demangle_symbol(mangled_name);
            auto        name_components = split(demangled_name, "::");
            auto        symbol_name     = name_components.back();

            enum { NOT_INCLUDED, INCLUDED, EXCLUDED, FORCE_INCLUDED, FORCE_EXCLUDED } state = NOT_INCLUDED;


            // Check if the symbol is force included or force excluded.
            for (const auto& rgx : args_yo) {
                if (std::regex_match(symbol_name.begin(), symbol_name.end(), rgx)) {
                    state = FORCE_INCLUDED;
                    break;
                }
            }

            for (const auto& rgx : args_no) {
                if (std::regex_match(symbol_name.begin(), symbol_name.end(), rgx)) {
                    state = FORCE_EXCLUDED;
                    break;
                }
            }


            // If the symbol is not force included or excluded, check the include status of the namespace.
            // TODO: Cache namespace status?
            if (state == NOT_INCLUDED) {
                for (const auto& ns : name_components | views::drop_last(1)) {
                    if (state == NOT_INCLUDED && !args_y.empty()) {
                        for (const auto& rgx : args_y) {
                            if (std::regex_match(ns.begin(), ns.end(), rgx)) {
                                state = INCLUDED;
                                break;
                            }
                        }
                    }

                    if ((state == NOT_INCLUDED || state == INCLUDED) && !args_n.empty()) {
                        for (const auto& rgx : args_n) {
                            if (std::regex_match(ns.begin(), ns.end(), rgx)) {
                                state = EXCLUDED;
                                break;
                            }
                        }
                    }
                }
            }


            if (state == INCLUDED || state == FORCE_INCLUDED) {
                included_symbols.emplace_back(mangled_name);
                cached_symbols.emplace(std::move(mangled_name), true);
            } else {
                cached_symbols.emplace(std::move(mangled_name), false);
            }

            has_uncached_symbols = true;
        }


        log.verbose("Keeping ", included_symbols.size(), "/", reader.get_symbols()->size(), " symbols");
    }


    void translation_unit_processor::load_cache(const fs::path& path) {
        if (!fs::exists(path)) return;

        enum { UNKNOWN, READ_SETTINGS, READ_SYMBOLS } state = UNKNOWN;
        std::ifstream stream { path };
        log.assert_that(!stream.fail(), "Failed to read cache file ", path);

        std::string line;
        while (std::getline(stream, line)) {
            std::string_view sv { line };
            if (sv.empty()) continue;

            if (sv.starts_with("#SETTINGS")) {
                state = READ_SETTINGS;
                continue;
            }

            if (sv.starts_with("#SYMBOLS")) {
                state = READ_SYMBOLS;
                continue;
            }

            if (state == READ_SETTINGS) {
                auto pos = sv.find('=');
                log.assert_that(pos != std::string_view::npos, "Incorrectly formatted cache file: ", path);

                auto k   = sv.substr(0, pos);
                auto v   = sv.substr(pos, std::string_view::npos);

                // Make sure setting from cache is also set currently.
                auto current_setting = argument_parser::instance().template get_argument<std::string>(k);
                if (!current_setting) return; // Settings mismatch, can't use cache.

                // Make sure setting has the same value. Value is space-separated list of strings, but can be re-ordered.
                auto current_elems = split(*current_setting, " ");
                std::size_t matched_elems = 0;

                for (const auto& setting : split(v, " ")) {
                    if (auto it = std::ranges::find(current_elems, setting); it == current_elems.end()) return; // Settings mismatch, can't use cache.
                    ++matched_elems;
                }

                if (matched_elems != current_elems.size()) return; // Settings mismatch, can't use cache.
            }

            if (state == READ_SYMBOLS) {
                auto pos = sv.find('=');
                log.assert_that(pos != std::string_view::npos, "Incorrectly formatted cache file: ", path);

                auto k   = sv.substr(0, pos);
                auto v   = sv.substr(pos, std::string_view::npos);

                cached_symbols.emplace(k, (v == "T"));
            }
        }

        log.verbose("Loaded ", cached_symbols.size(), " symbols from cache.");
    }


    void translation_unit_processor::write_cache(const fs::path& path) const {
        std::ofstream stream { path };

        stream << "#SETTINGS\n";
        for (const auto& setting : std::array { "y", "n", "yo", "no" }) {
            if (auto arg = argument_parser::instance().template get_argument<std::string>(setting); arg) {
                stream << setting << "=" << *arg << "\n";
            }
        }

        stream << "#SYMBOLS\n";
        for (const auto& [symbol, enabled] : cached_symbols) {
            stream << symbol << "=" << (enabled ? 'T' : 'F') << "\n";
        }

        log.assert_that((bool) stream, "Failed to write to cache file ", path);
        log.verbose("Wrote ", cached_symbols.size(), " symbols to cache.");
    }
}