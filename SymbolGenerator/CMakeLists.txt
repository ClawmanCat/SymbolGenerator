include(create_target)


create_target(
    SymbolGenerator
    EXECUTABLE
    0 0 1
    # Dependencies:
    CONAN_PKG::abseil
    CONAN_PKG::COFFI
    CONAN_PKG::range-v3
)