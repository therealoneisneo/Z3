/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    sat_tactic.cpp

Abstract:

    Tactic for using the SAT solver and its preprocessing capabilities.
    
Author:

    Leonardo (leonardo) 2011-10-25

Notes:

--*/
#include"tactical.h"
#include"goal2sat.h"
#include"sat_solver.h"
#include"filter_model_converter.h"
#include"ast_smt2_pp.h"
#include"model_v2_pp.h"

class sat_tactic : public tactic {

    struct imp {
        ast_manager &   m;
        goal2sat        m_goal2sat;
        sat2goal        m_sat2goal;
        sat::solver     m_solver;
        params_ref      m_params;
        
        imp(ast_manager & _m, params_ref const & p):
            m(_m),
            m_solver(p, 0),
            m_params(p) {
            SASSERT(!m.proofs_enabled());
        }
        
        void operator()(goal_ref const & g, 
                        goal_ref_buffer & result, 
                        model_converter_ref & mc, 
                        proof_converter_ref & pc,
                        expr_dependency_ref & core) {
            mc = 0; pc = 0; core = 0;
            fail_if_proof_generation("sat", g);
            fail_if_unsat_core_generation("sat", g);
            bool produce_models = g->models_enabled();
            TRACE("before_sat_solver", g->display(tout););
            g->elim_redundancies();

            atom2bool_var map(m);
            m_goal2sat(*g, m_params, m_solver, map);
            TRACE("sat_solver_unknown", tout << "interpreted_atoms: " << map.interpreted_atoms() << "\n";
                  atom2bool_var::iterator it  = map.begin();
                  atom2bool_var::iterator end = map.end();
                  for (; it != end; ++it) {
                      if (!is_uninterp_const(it->m_key))
                          tout << mk_ismt2_pp(it->m_key, m) << "\n";
                  });
            g->reset();
            g->m().compact_memory();

            CASSERT("sat_solver", m_solver.check_invariant());
            IF_VERBOSE(TACTIC_VERBOSITY_LVL, m_solver.display_status(verbose_stream()););
            TRACE("sat_dimacs", m_solver.display_dimacs(tout););
            
            lbool r = m_solver.check();
            if (r == l_false) {
                g->assert_expr(m.mk_false(), 0, 0);
            }
            else if (r == l_true && !map.interpreted_atoms()) {
                // register model
                if (produce_models) {
                    model_ref md = alloc(model, m);
                    sat::model const & ll_m = m_solver.get_model();
                    TRACE("sat_tactic", for (unsigned i = 0; i < ll_m.size(); i++) tout << i << ":" << ll_m[i] << " "; tout << "\n";);
                    atom2bool_var::iterator it  = map.begin();
                    atom2bool_var::iterator end = map.end();
                    for (; it != end; ++it) {
                        expr * n   = it->m_key;
                        sat::bool_var v = it->m_value;
                        TRACE("sat_tactic", tout << "extracting value of " << mk_ismt2_pp(n, m) << "\nvar: " << v << "\n";);
                        switch (sat::value_at(v, ll_m)) {
                        case l_true: 
                            md->register_decl(to_app(n)->get_decl(), m.mk_true()); 
                            break;
                        case l_false:
                            md->register_decl(to_app(n)->get_decl(), m.mk_false());
                            break;
                        default:
                            break;
                        }
                    }
                    TRACE("sat_tactic", model_v2_pp(tout, *md););
                    mc = model2model_converter(md.get());
                }
            }
            else {
                // get simplified problem.
#if 0
                IF_VERBOSE(TACTIC_VERBOSITY_LVL, verbose_stream() << "\"formula constains interpreted atoms, recovering formula from sat solver...\"\n";);
#endif
                m_solver.pop(m_solver.scope_lvl());
                m_sat2goal(m_solver, map, m_params, *(g.get()), mc);
            }
            g->inc_depth();
            result.push_back(g.get());
        }
        
        void set_cancel(bool f) {
            m_goal2sat.set_cancel(f);
            m_sat2goal.set_cancel(f);
            m_solver.set_cancel(f);
        }
    };
    
    struct scoped_set_imp {
        sat_tactic * m_owner; 

        scoped_set_imp(sat_tactic * o, imp * i):m_owner(o) {
            #pragma omp critical (sat_tactic)
            {
                m_owner->m_imp = i;
            }
        }
        
        ~scoped_set_imp() {
            #pragma omp critical (sat_tactic)
            {
                m_owner->m_imp = 0;
            }
        }
    };

    imp *      m_imp;
    params_ref m_params;
    statistics m_stats;

public:
    sat_tactic(ast_manager & m, params_ref const & p):
        m_imp(0),
        m_params(p) {
    }

    virtual tactic * translate(ast_manager & m) {
        return alloc(sat_tactic, m, m_params);
    }

    virtual ~sat_tactic() {
        SASSERT(m_imp == 0);
    }

    virtual void updt_params(params_ref const & p) {
        m_params = p;
    }

    virtual void collect_param_descrs(param_descrs & r) {
        goal2sat::collect_param_descrs(r);
        sat2goal::collect_param_descrs(r);
        sat::solver::collect_param_descrs(r);
    }
    
    void operator()(goal_ref const & g, 
                    goal_ref_buffer & result, 
                    model_converter_ref & mc, 
                    proof_converter_ref & pc,
                    expr_dependency_ref & core) {
        imp proc(g->m(), m_params);
        scoped_set_imp set(this, &proc);
        try {
            proc(g, result, mc, pc, core);
            proc.m_solver.collect_statistics(m_stats);
        }
        catch (sat::solver_exception & ex) {
            proc.m_solver.collect_statistics(m_stats);
            throw tactic_exception(ex.msg());
        }
        TRACE("sat_stats", m_stats.display_smt2(tout););
    }

    virtual void cleanup() {
        SASSERT(m_imp == 0);
    }

    virtual void collect_statistics(statistics & st) const {
        st.copy(m_stats);
    }

    virtual void reset_statistics() {
        m_stats.reset();
    }

protected:
    virtual void set_cancel(bool f) {
        #pragma omp critical (sat_tactic)
        {
            if (m_imp)
                m_imp->set_cancel(f);
        }
    }

};

tactic * mk_sat_tactic(ast_manager & m, params_ref const & p) {
    return clean(alloc(sat_tactic, m, p));
}

tactic * mk_sat_preprocessor_tactic(ast_manager & m, params_ref const & p) {
    params_ref p_aux;
    p_aux.set_uint("max_conflicts", 0);
    tactic * t = clean(using_params(mk_sat_tactic(m, p), p_aux));
    t->updt_params(p);
    return t;
}

