#include <SymbolGenerator/utility.hpp>

#include <Windows.h>
#include <DbgHelp.h>

#include <mutex>


namespace symgen {
    std::string demangle_symbol(const std::string& symbol) {
        static std::array<char, (1 << 16)> symbol_buffer;
        static std::mutex mtx;
        std::lock_guard lock { mtx }; // DbgHelp functions are not threadsafe.

        DWORD count = UnDecorateSymbolName(
            (PCSTR) symbol.data(),
            (PSTR)  symbol_buffer.data(),
            (DWORD) (1 << 16),
            UNDNAME_NAME_ONLY
        );

        return std::string { symbol_buffer.begin(), symbol_buffer.begin() + count };
    }
}