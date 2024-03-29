/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    qfbv_tactic.cpp

Abstract:

    Tactic for QF_BV based on bit-blasting

Author:

    Leonardo (leonardo) 2012-02-22

Notes:

--*/
#include"tactical.h"
#include"simplify_tactic.h"
#include"propagate_values_tactic.h"
#include"solve_eqs_tactic.h"
#include"elim_uncnstr_tactic.h"
#include"smt_tactic.h"
#include"bit_blaster_tactic.h"
#include"bv1_blaster_tactic.h"
#include"max_bv_sharing_tactic.h"
#include"bv_size_reduction_tactic.h"
#include"aig_tactic.h"
#include"sat_tactic.h"

#define MEMLIMIT 300

tactic * mk_qfbv_tactic(ast_manager & m, params_ref const & p) {
    params_ref main_p;
    main_p.set_bool("elim_and", true);
    main_p.set_bool("push_ite_bv", true);
    main_p.set_bool("blast_distinct", true);

    params_ref simp2_p = p;
    simp2_p.set_bool("som", true);
    simp2_p.set_bool("pull_cheap_ite", true);
    simp2_p.set_bool("push_ite_bv", false);
    simp2_p.set_bool("local_ctx", true);
    simp2_p.set_uint("local_ctx_limit", 10000000);
    simp2_p.set_bool("flat", true); // required by som
    simp2_p.set_bool("hoist_mul", false); // required by som

    params_ref local_ctx_p = p;
    local_ctx_p.set_bool("local_ctx", true);

    params_ref solver_p;
    solver_p.set_bool("preprocess", false); // preprocessor of smt::context is not needed.
    
    params_ref no_flat_p;
    no_flat_p.set_bool("flat", false);

    params_ref ctx_simp_p;
    ctx_simp_p.set_uint("max_depth", 32);
    ctx_simp_p.set_uint("max_steps", 50000000);

    params_ref hoist_p;
    hoist_p.set_bool("hoist_mul", true);
    hoist_p.set_bool("som", false);

    params_ref solve_eq_p;
    // conservative guassian elimination. 
    solve_eq_p.set_uint("solve_eqs_max_occs", 2); 

    params_ref big_aig_p;
    big_aig_p.set_bool("aig_per_assertion", false);

    tactic * preamble_st = and_then(and_then(mk_simplify_tactic(m),
                                             mk_propagate_values_tactic(m),
                                             using_params(mk_solve_eqs_tactic(m), solve_eq_p),
                                             mk_elim_uncnstr_tactic(m),
                                             if_no_proofs(if_no_unsat_cores(mk_bv_size_reduction_tactic(m))),
                                             using_params(mk_simplify_tactic(m), simp2_p)),
                                    // Z3 can solve a couple of extra benchmarks by using hoist_mul
                                    // but the timeout in SMT-COMP is too small. 
                                    // Moreover, it impacted negatively some easy benchmarks.
                                    // We should decide later, if we keep it or not.
                                    using_params(mk_simplify_tactic(m), hoist_p),
                                    mk_max_bv_sharing_tactic(m));
    
#ifdef USE_OLD_SAT_SOLVER
    tactic * new_sat = and_then(mk_simplify_tactic(m),
                                mk_smt_tactic());
#else
    tactic * new_sat = cond(mk_or(mk_produce_proofs_probe(), mk_produce_unsat_cores_probe()),
                            and_then(mk_simplify_tactic(m),
                                     mk_smt_tactic()),
                            mk_sat_tactic(m));
#endif    
    
    tactic * st = using_params(and_then(preamble_st,
                                        // If the user sets HI_DIV0=false, then the formula may contain uninterpreted function
                                        // symbols. In this case, we should not use 
                                        cond(mk_is_qfbv_probe(),
                                             cond(mk_is_qfbv_eq_probe(),
                                                  and_then(mk_bv1_blaster_tactic(m),
                                                           using_params(mk_smt_tactic(), solver_p)),
                                                  and_then(mk_bit_blaster_tactic(m),
                                                           when(mk_lt(mk_memory_probe(), mk_const_probe(MEMLIMIT)),
                                                                and_then(using_params(and_then(mk_simplify_tactic(m),
                                                                                               mk_solve_eqs_tactic(m)),
                                                                                      local_ctx_p),
                                                                         if_no_proofs(cond(mk_produce_unsat_cores_probe(),
                                                                                           mk_aig_tactic(),
                                                                                           using_params(mk_aig_tactic(),
                                                                                                        big_aig_p))))),
                                                           new_sat)),
                                             mk_smt_tactic())),
                               main_p);

    st->updt_params(p);
    return st;
}


