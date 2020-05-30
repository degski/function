
// MIT License
//
// Copyright (c) 2020 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <array>
#include <atomic>
#include <algorithm>
#include <initializer_list>
#include <sax/iostream.hpp>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <jthread>
#include <type_traits>
#include <utility>
#include <vector>

/*
    -fsanitize = address

    C:\Program Files\LLVM\lib\clang\10.0.0\lib\windows\clang_rt.asan_cxx-x86_64.lib
    C:\Program Files\LLVM\lib\clang\10.0.0\lib\windows\clang_rt.asan-preinit-x86_64.lib
    C:\Program Files\LLVM\lib\clang\10.0.0\lib\windows\clang_rt.asan-x86_64.lib

    C:\Program Files (x86)\IntelSWTools\compilers_and_libraries\windows\tbb\lib\intel64_win\vc_mt\tbb.lib
*/

#include <sax/prng_sfc.hpp>
#include <sax/uniform_int_distribution.hpp>

#if defined( NDEBUG )
#    define RANDOM 1
#else
#    define RANDOM 0
#endif

namespace ThreadID {
// Creates a new ID.
[[nodiscard]] inline int get ( bool ) noexcept {
    static std::atomic<int> global_id = 0;
    return global_id++;
}
// Returns ID of this thread.
[[nodiscard]] inline int get ( ) noexcept {
    static thread_local int thread_local_id = get ( false );
    return thread_local_id;
}
} // namespace ThreadID

namespace Rng {
// Chris Doty-Humphrey's Small Fast Chaotic Prng.
[[nodiscard]] inline sax::Rng & generator ( ) noexcept {
    if constexpr ( RANDOM ) {
        static thread_local sax::Rng generator ( sax::os_seed ( ), sax::os_seed ( ), sax::os_seed ( ), sax::os_seed ( ) );
        return generator;
    }
    else {
        static thread_local sax::Rng generator ( sax::fixed_seed ( ) + ThreadID::get ( ) );
        return generator;
    }
}
} // namespace Rng

#undef RANDOM

sax::Rng & rng = Rng::generator ( );

// https://shaharmike.com/cpp/naive-std-function/

namespace sax {

template<typename T>
struct function;

template<typename ReturnValue, typename... Args>
struct function<ReturnValue ( Args... )> {

    template<typename T>
    [[maybe_unused]] function & operator= ( T && t_ ) noexcept {
        callable_ = std::make_unique<callable_type<T>> ( std::move ( t_ ) );
        return *this;
    }

    [[nodiscard]] ReturnValue operator( ) ( Args &&... args_ ) const {
        return callable_->operator( ) ( std::forward<Args> ( args_ )... );
    }

    struct call {
        inline virtual ~call ( )                       = 0;
        virtual ReturnValue operator( ) ( Args &&... ) = 0;
    };

    template<typename T>
    struct callable_type : public call {
        callable_type ( T && t_ ) noexcept : t ( std::move ( t_ ) ) {}
        ~callable_type ( ) override = default;
        virtual ReturnValue operator( ) ( Args &&... args ) override { return t ( std::forward<Args> ( args )... ); }
        T t;
    };

    template<typename U, typename... A>
    void construct ( U * p_, A &&... args_ ) {
        ::new ( p_ ) U ( std::forward<A> ( args_ )... );
    }
    template<typename U>
    void destroy ( U * p_ ) noexcept {
        p_->~U ( );
    }

    std::unique_ptr<call> callable_;
};

template<typename ReturnValue, typename... Args>
inline function<ReturnValue ( Args... )>::call::~call ( ){ };

} // namespace sax

void function ( ) { std::cout << "function" << nl; }

struct functor {
    void operator( ) ( ) { std::cout << "functor" << nl; }
};

int main ( ) {

    sax::function<void ( )> f;

    f = function;
    f ( );
    f = functor ( );
    f ( );
    f = [] ( ) { std::cout << "lambda" << nl; };
    f ( );

    return EXIT_SUCCESS;
}
