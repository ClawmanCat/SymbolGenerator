#include <coffi/coffi.hpp>


extern "C" __declspec(dllexport) int keep_symbol(const char* demangled_name, const void* symbol, const void* reader) {
    const COFFI::symbol& coffi_sym = *((const COFFI::symbol*) symbol);

    // Discard symbols that aren't functions (see https://www.delorie.com/djgpp/doc/coff/symtab.html).
    if ((coffi_sym.get_type() & 0b0011'0000) != (IMAGE_SYM_DTYPE_FUNCTION << 4)) return 0;

    // Discard symbols that have stupid names.
    for (const auto& name : { "blingbloing", "bingus", "bababooey" }) {
        if (strstr(demangled_name, name) != nullptr) return 0;
    }

    return 1;
}