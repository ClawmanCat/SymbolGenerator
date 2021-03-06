#pragma once

#include <type_traits>


// Abseil uses std::result_of, which was removed in C++20.
// TODO: Remove this once Abseil is updated as this is UB!
namespace std {
    template <typename T> class result_of {};

    template <typename Fn, typename... Args> struct result_of<Fn(Args...)> {
        using type = std::invoke_result_t<Fn, Args...>;
    };

    template <typename T> using result_of_t = typename std::result_of<T>::type;
}