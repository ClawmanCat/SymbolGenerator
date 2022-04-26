#pragma once

#include <coffi/coffi.hpp>

#include <array>
#include <string_view>
#include <optional>
#include <algorithm>


// Provides a set of filters to filter out any symbols that should never be exported, like scalar/vector deleting destructors and managed code.
// These filters are based on the way CMake performs filtering if WINDOWS_EXPORT_ALL_SYMBOLS is used (https://github.com/Kitware/CMake/blob/e3f2601a9d5854d34fec397f1d2c970af17bd5db/Source/bindexplib.cxx),
// which itself is based on the bindexplib tool from the CERN ROOT Data Analysis Framework project (https://root.cern.ch).
namespace symgen::filters {
    // Filter unexpected symbol types (Only types 0x00 and 0x20 should be present).
    extern bool filter_symbol_type(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // Filter scalar/vector deleting destructors.
    extern bool filter_destructors(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // Filter read-only constants.
    extern bool filter_constants(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // Filter function symbols that are not readable or executable.
    extern bool filter_rx_functions(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // Filter symbols containing a dot character.
    extern bool filter_dot_symbols(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // Filter symbols from managed code.
    extern bool filter_managed_code(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);
    // On ARM64EC, filter $i?[entry|exit]_thunk symbols.
    extern bool filter_arm64ec_thunk(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled);


    // Applies all filters to the given symbol. Returns the name of the filter that caused the symbol's exclusion, or nullopt otherwise.
    inline std::optional<std::string_view> apply_all(const COFFI::symbol& sym, const COFFI::coffi& reader, std::string_view demangled) {
        const static std::array filters {
            std::pair { &filter_symbol_type,   "filter_symbol_type"   },
            std::pair { &filter_destructors,   "filter_destructors"   },
            std::pair { &filter_constants,     "filter_constants"     },
            std::pair { &filter_rx_functions,  "filter_rx_functions"  },
            std::pair { &filter_dot_symbols,   "filter_dot_symbols"   },
            std::pair { &filter_managed_code,  "filter_managed_code"  },
            std::pair { &filter_arm64ec_thunk, "filter_arm64ec_thunk" }
        };


        for (auto [filter, name] : filters) {
            if (!std::invoke(filter, sym, reader, demangled)) return name;
        }

        return std::nullopt;
    }
}