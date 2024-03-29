/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    qflra_tactic.cpp

Abstract:

    Tactic for QF_LRA

Author:

    Leonardo (leonardo) 2012-02-26

Notes:

--*/
#include"tactical.h"
#include"simplify_tactic.h"
#include"propagate_values_tactic.h"
#include"solve_eqs_tactic.h"
#include"elim_uncnstr_tactic.h"
#include"smt_tactic.h"
// include"mip_tactic.h"
#include"recover_01_tactic.h"
#include"ctx_simplify_tactic.h"
#include"probe_arith.h"

tactic * mk_qflra_tactic(ast_manager & m, params_ref const & p) {
    params_ref pivot_p;
    pivot_p.set_bool("arith.greatest_error_pivot", true);

    params_ref main_p = p;
    main_p.set_bool("elim_and", true);
    main_p.set_bool("som", true);
    main_p.set_bool("blast_distinct", true);
    
    params_ref ctx_simp_p;
    ctx_simp_p.set_uint("max_depth", 30);
    ctx_simp_p.set_uint("max_steps", 5000000);
    
    params_ref lhs_p;
    lhs_p.set_bool("arith_lhs", true);
    lhs_p.set_bool("eq2ineq", true);
    
    params_ref elim_to_real_p;
    elim_to_real_p.set_bool("elim_to_real", true);
    

#if 0
    tactic * mip =
        and_then(fail_if(mk_produce_proofs_probe()),
                 fail_if(mk_produce_unsat_cores_probe()),
                 using_params(and_then(and_then(mk_simplify_tactic(m),
                                                mk_recover_01_tactic(m),
                                                using_params(mk_simplify_tactic(m), elim_to_real_p),
                                                mk_propagate_values_tactic(m)),
                                       using_params(mk_ctx_simplify_tactic(m), ctx_simp_p),
                                       mk_elim_uncnstr_tactic(m),
                                       mk_solve_eqs_tactic(m),
                                       using_params(mk_simplify_tactic(m), lhs_p),
                                       using_params(mk_simplify_tactic(m), elim_to_real_p)
                                       ),
                              main_p),
                 fail_if(mk_not(mk_is_mip_probe())),
                 try_for(mk_mip_tactic(m), 30000),
                 mk_fail_if_undecided_tactic());
#endif

    //    return using_params(or_else(mip,
    //                            using_params(mk_smt_tactic(), pivot_p)),
    //                    p);

    return using_params(using_params(mk_smt_tactic(), pivot_p), p);
}
