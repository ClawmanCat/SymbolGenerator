include(create_target)


create_target(
    ExampleFilter
    SHARED
    0 0 1
    # Dependencies:
    CONAN_PKG::COFFI
)