/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    cooperate.cpp

Abstract:

    Cooperation support

Author:

    Leonardo (leonardo) 2011-05-17

Notes:

--*/
#include"cooperate.h"
#include"trace.h"
#include"debug.h"
#include"z3_omp.h"

struct cooperation_lock {
    omp_nest_lock_t  m_lock;
    char const *     m_task;
    volatile int     m_owner_thread;
    cooperation_lock() {
        omp_set_nested(1);
        omp_init_nest_lock(&m_lock);
        m_task = 0;
        m_owner_thread = -1;
    }
    ~cooperation_lock() {
        omp_destroy_nest_lock(&m_lock);
    }
};

cooperation_lock g_lock;

bool cooperation_ctx::g_cooperate = false;

void cooperation_ctx::checkpoint(char const * task) {
    SASSERT(cooperation_ctx::enabled());

    int  tid      = omp_get_thread_num();
    if (g_lock.m_owner_thread == tid) {
        g_lock.m_owner_thread = -1;
        omp_unset_nest_lock(&(g_lock.m_lock));
    }
    // this critical section is used to force the owner thread to give a chance to
    // another thread to get the lock
    #pragma omp critical (z3_cooperate) 
    {
        omp_set_nest_lock(&(g_lock.m_lock));
        TRACE("cooperate_detail", tout << task << ", tid: " << tid << "\n";);
        CTRACE("cooperate", g_lock.m_task != task, tout << "moving to task: " << task << "\n";);
        g_lock.m_owner_thread = tid;
    }
}

cooperation_section::cooperation_section() {
    SASSERT(!cooperation_ctx::enabled());
    SASSERT(!omp_in_parallel());
    cooperation_ctx::g_cooperate = true;
}

cooperation_section::~cooperation_section() {
    SASSERT(cooperation_ctx::enabled());
    cooperation_ctx::g_cooperate = false;
}

init_task::init_task(char const * task) {
    SASSERT(cooperation_ctx::enabled());
    SASSERT(omp_in_parallel());
    cooperation_ctx::checkpoint(task);
}

init_task::~init_task() {
    int  tid      = omp_get_thread_num();
    if (g_lock.m_owner_thread == tid) {
        g_lock.m_owner_thread = -1;
        omp_unset_nest_lock(&(g_lock.m_lock));
    }
}


