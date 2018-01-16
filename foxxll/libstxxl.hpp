/***************************************************************************
 *  foxxll/libstxxl.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_LIBSTXXL_HEADER
#define FOXXLL_LIBSTXXL_HEADER

#include <foxxll/config.hpp>

#if FOXXLL_MSVC
 #ifndef STXXL_LIBNAME
  #define STXXL_LIBNAME "stxxl"
 #endif
//-tb #pragma comment (lib, "lib" STXXL_LIBNAME ".lib")
#endif

#endif // !FOXXLL_LIBSTXXL_HEADER