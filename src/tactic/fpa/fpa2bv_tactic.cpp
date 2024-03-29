/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    fpa2bv_tactic.cpp

Abstract:

    Tactic that converts floating points to bit-vectors

Author:

    Christoph (cwinter) 2012-02-09

Notes:

--*/
#include"tactical.h"
#include"fpa2bv_rewriter.h"
#include"simplify_tactic.h"
#include"fpa2bv_tactic.h"
#include"fpa2bv_model_converter.h"

class fpa2bv_tactic : public tactic {
    struct imp {
        ast_manager &     m;
        fpa2bv_converter  m_conv;
        fpa2bv_rewriter   m_rw;
        unsigned          m_num_steps;

        bool              m_proofs_enabled;
        bool              m_produce_models;
        bool              m_produce_unsat_cores;

        imp(ast_manager & _m, params_ref const & p):
            m(_m),
            m_conv(m),
            m_rw(m, m_conv, p),
            m_proofs_enabled(false),
            m_produce_models(false),
            m_produce_unsat_cores(false) {
            }

        void updt_params(params_ref const & p) {
            m_rw.cfg().updt_params(p);        
        }
        
        void set_cancel(bool f) {
            m_rw.set_cancel(f);
        }

        virtual void operator()(goal_ref const & g, 
                                goal_ref_buffer & result, 
                                model_converter_ref & mc, 
                                proof_converter_ref & pc,
                                expr_dependency_ref & core) {
            SASSERT(g->is_well_sorted());
            fail_if_proof_generation("fpa2bv", g);
            fail_if_unsat_core_generation("fpa2bv", g);
            m_proofs_enabled      = g->proofs_enabled();
            m_produce_models      = g->models_enabled();
            m_produce_unsat_cores = g->unsat_core_enabled();
            
            mc = 0; pc = 0; core = 0; result.reset();
            tactic_report report("fpa2bv", *g);
            m_rw.reset(); 

            TRACE("fpa2bv", tout << "BEFORE: " << std::endl; g->display(tout););

            if (g->inconsistent()) {
                result.push_back(g.get());
                return;
            }
            
            m_num_steps = 0;
            expr_ref   new_curr(m);
            proof_ref  new_pr(m);
            unsigned size = g->size();
            for (unsigned idx = 0; idx < size; idx++) {
                if (g->inconsistent())
                    break;
                expr * curr = g->form(idx);
                m_rw(curr, new_curr, new_pr);
                m_num_steps += m_rw.get_num_steps();
                if (m_proofs_enabled) {
                    proof * pr = g->pr(idx);
                    new_pr     = m.mk_modus_ponens(pr, new_pr);
                }
                g->update(idx, new_curr, new_pr, g->dep(idx));
            }

            if (g->models_enabled())  
                mc = mk_fpa2bv_model_converter(m, m_conv.const2bv(), m_conv.rm_const2bv(), m_conv.uf2bvuf(), m_conv.uf23bvuf());

            g->inc_depth();
            result.push_back(g.get());

            for (unsigned i = 0; i < m_conv.extra_assertions.size(); i++)
                result.back()->assert_expr(m_conv.extra_assertions[i].get());

            SASSERT(g->is_well_sorted());
            TRACE("fpa2bv", tout << "AFTER: " << std::endl; g->display(tout); 
                            if (mc) mc->display(tout); tout << std::endl; );
        }
    };

    imp *      m_imp;
    params_ref m_params;

public:
    fpa2bv_tactic(ast_manager & m, params_ref const & p):
        m_params(p) {
        m_imp = alloc(imp, m, p);
    }

    virtual tactic * translate(ast_manager & m) {
        return alloc(fpa2bv_tactic, m, m_params);
    }

    virtual ~fpa2bv_tactic() {
        dealloc(m_imp);
    }

    virtual void updt_params(params_ref const & p) {
        m_params = p;
        m_imp->updt_params(p);
    }

    virtual void collect_param_descrs(param_descrs & r) {        
    }

    virtual void operator()(goal_ref const & in, 
                            goal_ref_buffer & result, 
                            model_converter_ref & mc, 
                            proof_converter_ref & pc,
                            expr_dependency_ref & core) {
        (*m_imp)(in, result, mc, pc, core);
    }
    
    virtual void cleanup() {        
        imp * d = alloc(imp, m_imp->m, m_params);
        #pragma omp critical (tactic_cancel)
        {
            std::swap(d, m_imp);
        }
        dealloc(d);
    }

protected:
    virtual void set_cancel(bool f) {
        if (m_imp)
            m_imp->set_cancel(f);
    }
};

tactic * mk_fpa2bv_tactic(ast_manager & m, params_ref const & p) {    
    return clean(alloc(fpa2bv_tactic, m, p));
}
