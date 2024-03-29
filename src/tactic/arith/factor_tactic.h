/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    factor_tactic.h

Abstract:

    Polynomial factorization tactic.

Author:

    Leonardo de Moura (leonardo) 2012-02-03

Revision History:

--*/
#ifndef _FACTOR_TACTIC_H_
#define _FACTOR_TACTIC_H_

#include"params.h"
class ast_manager;
class tactic;

tactic * mk_factor_tactic(ast_manager & m, params_ref const & p = params_ref());
/*
  ADD_TACTIC("factor", "polynomial factorization.", "mk_factor_tactic(m, p)")
*/
#endif
