#include <coffi/coffi.hpp>


extern "C" __declspec(dllexport) int keep_symbol(const char* demangled_name, const void* symbol, const void* reader) {
    // Discard symbols that have stupid names.
    for (const auto& name : { "blingbloing", "bingus", "bababooey" }) {
        if (strstr(demangled_name, name) != nullptr) return 0;
    }

    return 1;
}