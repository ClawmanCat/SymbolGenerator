#include <SymbolGenerator/unexported_symbol_filters.hpp>
#include <SymbolGenerator/coff_utils.hpp>
#include <SymbolGenerator/defs.hpp>

#include <regex>


namespace symgen::filters {
    std::string remove_prefix(std::string_view mangled_name, const COFFI::coffi& reader) {
        std::string result { mangled_name };

        std::size_t leading_whitespace = 0;
        while (result.size() > leading_whitespace && std::isspace(result[leading_whitespace])) ++leading_whitespace;
        result.erase(0, leading_whitespace);

        if (result.starts_with('_')) {
            auto where = result.find('@');
            if (where != std::string::npos) result.erase(where);
        }

        if (reader.get_header()->get_machine() == IMAGE_FILE_MACHINE_I386) {
            if (result.starts_with('_')) result.erase(0, 1);
        }

        return result;
    }


    bool filter_symbol_type(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        return is_data_symbol(sym) || is_function_symbol(sym);
    }


    bool filter_destructors(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        auto base_name = remove_prefix(sym.get_name(), reader);

        if (base_name.starts_with("??_G") || base_name.starts_with("??_E")) return false;
        return true;
    }


    bool filter_constants(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        return !(sym.get_type() == IMAGE_SYM_TYPE_NULL && !(get_section_flags_for_symbol(sym, reader) & SECTION_WRITE_BIT));
    }


    bool filter_rx_functions(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        return !(sym.get_type() == IMAGE_SYM_DTYPE_FUNCTION && !(get_section_flags_for_symbol(sym, reader) & (SECTION_READ_BIT | SECTION_EXECUTE_BIT)));
    }


    bool filter_dot_symbols(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        return sym.get_name().find('.') == std::string::npos;
    }


    bool filter_managed_code(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        auto base_name = remove_prefix(sym.get_name(), reader);

        if (base_name.find("$$F") != std::string::npos || base_name.find("$$J") != std::string::npos) return false;
        if (ranges::contains(std::array { "__t2m", "__m2mep", "__mep" }, base_name)) return false;

        return true;
    }


    bool filter_arm64ec_thunk(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        if (reader.get_header()->get_machine() == IMAGE_FILE_MACHINE_ARM64EC) {
            const static std::regex filter { "\\$i?(entry|exit)_thunk" };
            auto base_name = remove_prefix(sym.get_name(), reader);

            if (std::regex_match(base_name, filter)) return false;
        }

        return true;
    }
}