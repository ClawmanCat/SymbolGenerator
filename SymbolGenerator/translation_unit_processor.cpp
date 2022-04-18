#include <SymbolGenerator/translation_unit_processor.hpp>
#include <SymbolGenerator/rule_cache.hpp>
#include <SymbolGenerator/utility.hpp>
#include <SymbolGenerator/logger.hpp>

#include <coffi/coffi.hpp>
#include <coffi/coffi_types.hpp>

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
        COFFI::coffi reader;
        reader.load(obj_path.string());
        log.verbose(reader.get_symbols()->size(), " symbols found.");


        const auto& args_y  = rule_cache::instance().get_includes();
        const auto& args_n  = rule_cache::instance().get_excludes();
        const auto& args_yo = rule_cache::instance().get_force_includes();
        const auto& args_no = rule_cache::instance().get_force_excludes();

        filter_function filter_fn = argument_parser::instance().has_argument("fn")
            ? load_filter_function(*argument_parser::instance().get_argument<std::string>("fn"))
            : [] (const char*, const void*, const void*) { return 1; };


        for (const auto& sym : *reader.get_symbols()) {
            if (sym.get_type() == IMAGE_SYM_TYPE_NULL) continue;
            std::string mangled_name = sym.get_name();


            if (auto it = cached_symbols.find(mangled_name); it != cached_symbols.end()) {
                if (it->second) included_symbols.push_back(std::move(mangled_name));
                continue;
            }


            std::string demangled_name  = demangle_symbol(mangled_name);
            auto        name_components = split(demangled_name, "::");

            enum { NOT_INCLUDED, INCLUDED, EXCLUDED, FORCE_INCLUDED, FORCE_EXCLUDED } state = NOT_INCLUDED;


            // Check if the symbol is force included or force excluded.
            for (const auto& rgx : args_yo) {
                if (std::regex_match(demangled_name.begin(), demangled_name.end(), rgx)) {
                    state = FORCE_INCLUDED;
                    break;
                }
            }

            for (const auto& rgx : args_no) {
                if (std::regex_match(demangled_name.begin(), demangled_name.end(), rgx)) {
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


            // If the symbol is included, check if it isn't excluded by the filter function.
            if (state == INCLUDED || state == FORCE_INCLUDED) {
                if (filter_fn(sym.get_name().c_str(), &sym, &reader) == 0) state = FORCE_EXCLUDED;
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
        if (!fs::exists(path)) {
            log.verbose("No cache found.");
            return;
        }

        enum { UNKNOWN, READ_SETTINGS, READ_SYMBOLS } state = UNKNOWN;
        std::ifstream stream { path };
        log.assert_that(!stream.fail(), "Failed to read cache file ", path);

        std::string line;
        hash_map<std::string, std::string> cached_args;

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

                auto k = sv.substr(0, pos);
                auto v = sv.substr(pos + 1, std::string_view::npos);
                cached_args.emplace(k, v);
            }

            if (state == READ_SYMBOLS) {
                auto pos = sv.find('=');
                log.assert_that(pos != std::string_view::npos, "Incorrectly formatted cache file: ", path);

                auto k = sv.substr(0, pos);
                auto v = sv.substr(pos + 1, std::string_view::npos);

                cached_symbols.emplace(k, (v == "T"));
            }
        }


        // Make sure arguments match between cache and current program invocation.
        hash_map<std::string, std::string> current_args;
        for (const auto& setting : std::array { "y", "n", "yo", "no", "fn" }) {
            if (auto arg = argument_parser::instance().get_argument<std::string>(setting); arg) {
                current_args.emplace(setting, *arg);
            }
        }

        auto error_msg = check_settings_compatible(cached_args, current_args);
        if (error_msg) {
            log.verbose("Cache is out of date and cannot be used. (", *error_msg, ")");
            cached_symbols.clear();
        } else {
            log.verbose("Loaded ", cached_symbols.size(), " symbols from cache.");
        }
    }


    void translation_unit_processor::write_cache(const fs::path& path) const {
        std::ofstream stream { path };

        stream << "#SETTINGS\n";
        for (const auto& setting : std::array { "y", "n", "yo", "no", "fn" }) {
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