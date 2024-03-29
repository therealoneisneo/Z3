/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    ufbv_rewriter_tactic.cpp

Abstract:

    UFBV Rewriter (demodulator)

Author:

    Christoph (cwinter) 2012-10-26

Notes:

--*/
#ifndef _UFBV_REWRITER_TACTIC_H_
#define _UFBV_REWRITER_TACTIC_H_

#include"params.h"
class ast_manager;
class tactic;

tactic * mk_ufbv_rewriter_tactic(ast_manager & m, params_ref const & p = params_ref());

#endif
