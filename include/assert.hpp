//=======================================================================
// Copyright (c) 2013-2014 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef BUDGET_ASSERT_HPP
#define BUDGET_ASSERT_HPP

#include <iostream>

#include "likely.hpp"

#ifdef NDEBUG

#define budget_assert(condition, message) ((void)0)

#if defined __clang__

#if __has_builtin(__builtin_unreachable)
#define budget_unreachable(message) __builtin_unreachable();
#else
#define budget_unreachable(message) ((void)0)
#endif //__has_builtin(__builtin_unreachable)

#elif defined __GNUC__

#define budget_unreachable(message) __builtin_unreachable();

#endif //__clang__

#else

#define budget_assert(condition, message) (likely(condition) \
    ? ((void)0) \
    : ::budget::assertion::detail::assertion_failed_msg(#condition, message, \
    __PRETTY_FUNCTION__, __FILE__, __LINE__))

#if defined __clang__

#if __has_builtin(__builtin_unreachable)
#define budget_unreachable(message) budget_assert(false, message); __builtin_unreachable();
#else
#define budget_unreachable(message) budget_assert(false, message);
#endif //__has_builtin(__builtin_unreachable)

#elif defined __GNUC__

#define budget_unreachable(message) budget_assert(false, message); __builtin_unreachable();

#endif //__clang__

#endif //NDEBUG

namespace budget {
namespace assertion {
namespace detail {

    template< typename CharT >
    void assertion_failed_msg(const CharT* expr, const char* msg, const char* function, const char* file, long line){
        std::cerr
            << "***** Internal Program Error - assertion (" << expr << ") failed in "
            << function << ":\n"
            << file << '(' << line << "): " << msg << std::endl;
        std::abort();
    }

} // end of detail namespace
} // end of assertion namespace
} // end of detail namespace

#endif //BUDGET_ASSERT_HPP
