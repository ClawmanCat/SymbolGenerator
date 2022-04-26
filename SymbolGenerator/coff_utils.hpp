#pragma once

#include <SymbolGenerator/defs.hpp>

#include <coffi/coffi.hpp>


namespace symgen {
    constexpr std::uint32_t IMAGE_FILE_MACHINE_ARM64EC = 0xA641;

    constexpr std::uint32_t SECTION_READ_BIT    = 0x40000000;
    constexpr std::uint32_t SECTION_WRITE_BIT   = 0x80000000;
    constexpr std::uint32_t SECTION_EXECUTE_BIT = 0x20000000;


    inline std::uint32_t get_section_flags_for_symbol(const COFFI::symbol& sym, const COFFI::coffi& reader) {
        using u16 = std::uint16_t;

        auto section_id = (u16) sym.get_section_number();
        if (ranges::contains(std::array { (u16) IMAGE_SYM_UNDEFINED, (u16) IMAGE_SYM_ABSOLUTE, (u16) IMAGE_SYM_DEBUG }, section_id)) return 0;

        if (section_id >= reader.get_sections().size()) return 0;

        const auto& section = reader.get_sections().at(section_id);
        return section->get_flags();
    }


    inline bool is_data_symbol(const COFFI::symbol& sym) {
        // Microsoft tools use the type field only to indicate whether or not the symbol is a function,
        // so the only possible values are 0x00 (SYM_TYPE_NULL) and 0x20 (SYM_DTYPE_FUNCTION).
        return sym.get_type() == IMAGE_SYM_TYPE_NULL;
    }


    inline bool is_function_symbol(const COFFI::symbol& sym) {
        // Microsoft tools use the type field only to indicate whether or not the symbol is a function,
        // so the only possible values are 0x00 (SYM_TYPE_NULL) and 0x20 (SYM_DTYPE_FUNCTION).
        return sym.get_type() == (IMAGE_SYM_DTYPE_FUNCTION << 4);
    }
}