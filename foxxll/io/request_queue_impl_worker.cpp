/***************************************************************************
 *  foxxll/io/request_queue_impl_worker.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cassert>
#include <cstddef>
#include <thread>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/common/shared_state.hpp>
#include <foxxll/config.hpp>
#include <foxxll/io/request_queue_impl_worker.hpp>
#include <tlx/semaphore.hpp>

#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
 #include <windows.h>
#endif

namespace foxxll {

void request_queue_impl_worker::start_thread(
    void* (*worker)(void*), void* arg, std::thread& t,
    shared_state<thread_state>& s)
{
    assert(s() == NOT_RUNNING);
    t = std::thread(worker, arg);
    s.set_to(RUNNING);
}

void request_queue_impl_worker::stop_thread(
    std::thread& t, shared_state<thread_state>& s, tlx::semaphore& sem)
{
    assert(s() == RUNNING);
    s.set_to(TERMINATING);
    sem.signal();
#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
    // In the Visual C++ Runtime 2012 and 2013, there is a deadlock bug, which
    // occurs when threads are joined after main() exits. Apparently, Microsoft
    // thinks this is not a big issue. It has not been fixed in VC++RT 2013.
    // https://connect.microsoft.com/VisualStudio/feedback/details/747145
    //
    // All FOXXLL threads are created by singletons, which are global variables
    // that are deleted after main() exits. The fix applied here it to use
    // std::thread::native_handle() and access the WINAPI to terminate the
    // thread directly (after it finished handling its i/o requests).

    WaitForSingleObject(t.native_handle(), INFINITE);
    CloseHandle(t.native_handle());
#else
    t.join();
#endif
    assert(s() == TERMINATED);
    s.set_to(NOT_RUNNING);
}

} // namespace foxxll

/**************************************************************************/
