#include <SymbolGenerator/utility.hpp>

#include <Windows.h>
#include <DbgHelp.h>

#include <mutex>


namespace symgen {
    std::string get_last_winapi_error(void) {
        DWORD error_code     = GetLastError();
        LPSTR message_buffer = nullptr;


        if (error_code == 0) return "No error";

        auto count = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM     |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &message_buffer,
            0,
            nullptr
        );

        std::string result { message_buffer, message_buffer + count };

        LocalFree(message_buffer);
        return result;
    }


    filter_function load_filter_function(const fs::path& path) {
        struct dll_raii_lifetime {
            HINSTANCE dll_handle;
            ~dll_raii_lifetime(void) { FreeLibrary(dll_handle); }
        };


        static dll_raii_lifetime lifetime { [&] {
            std::string path_str  = path.string();
            LPCSTR      path_cstr = path_str.c_str();

            auto result = LoadLibrary(path_cstr);
            logger::instance().assert_that(result, "Failed to load DLL ", path, ": ", get_last_winapi_error());
            logger::instance().verbose("Loaded ", path, " as additional filter function.");

            return result;
        } () };


        auto result = GetProcAddress(lifetime.dll_handle, "keep_symbol");
        logger::instance().assert_that(result, "Failed to load keep_symbol method from DLL ", path, ": ", get_last_winapi_error());

        return (filter_function) result;
    }


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