/***************************************************************************
 *  foxxll/unused.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_UNUSED_HEADER
#define FOXXLL_UNUSED_HEADER

namespace foxxll {

template <typename U>
inline void STXXL_UNUSED(const U&)
{ }

} // namespace foxxll

#endif // !FOXXLL_UNUSED_HEADER
// vim: et:ts=4:sw=4