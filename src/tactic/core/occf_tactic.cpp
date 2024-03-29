/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    occf_tactic.cpp

Abstract:

    Put clauses in the assertion set in
    OOC (one constraint per clause) form.
    Constraints occuring in formulas that
    are not clauses are ignored.
    The formula can be put into CNF by
    using mk_sat_preprocessor strategy.

Author:

    Leonardo de Moura (leonardo) 2011-12-28.

Revision History:

--*/
#include"tactical.h"
#include"occf_tactic.h"
#include"filter_model_converter.h"
#include"cooperate.h"

class occf_tactic : public tactic {
    struct     imp {
        ast_manager &            m;
        volatile bool            m_cancel;
        filter_model_converter * m_mc;
        
        imp(ast_manager & _m):
            m(_m) {
            m_cancel = false;
        }

        void set_cancel(bool f) {
            m_cancel = f;
        }

        void checkpoint() {
            if (m_cancel)
                throw tactic_exception(TACTIC_CANCELED_MSG);
            cooperate("occf");
        }

        bool is_literal(expr * t) const {
            expr * atom;
            return is_uninterp_const(t) || (m.is_not(t, atom) && is_uninterp_const(atom));
        }
        
        bool is_constraint(expr * t) const {
            return !is_literal(t);
        }
        
        bool is_target(app * cls) {
            SASSERT(m.is_or(cls));
            bool found = false;
            unsigned num = cls->get_num_args();
            for (unsigned i = 0; i < num; i++) {
                if (is_constraint(cls->get_arg(i))) {
                    if (found)
                        return true;
                    found = true;
                }
            }
            return false;
        }
        
        struct bvar_info {
            expr *   m_bvar;
            unsigned m_gen_pos:1;
            unsigned m_gen_neg:1;
            bvar_info():m_bvar(0), m_gen_pos(false), m_gen_neg(false) {}
            bvar_info(expr * var, bool sign):
                m_bvar(var),
                                            m_gen_pos(!sign),
                m_gen_neg(sign) {
            }
        };
        
        typedef obj_map<expr, bvar_info> cnstr2bvar;
        
        expr * get_aux_lit(cnstr2bvar & c2b, expr * cnstr, goal_ref const & g) {
            bool sign = false;
            while (m.is_not(cnstr)) {
                cnstr = to_app(cnstr)->get_arg(0);
                sign  = !sign;
            }
            
            cnstr2bvar::obj_map_entry * entry = c2b.find_core(cnstr);
            if (entry == 0)
                return 0;
            bvar_info & info = entry->get_data().m_value;
            if (sign) {
                if (!info.m_gen_neg) {
                    info.m_gen_neg = true;
                    g->assert_expr(m.mk_or(info.m_bvar, m.mk_not(cnstr)), 0, 0);
                }
                return m.mk_not(info.m_bvar);
            }
            else {
                if (!info.m_gen_pos) {
                    info.m_gen_pos = true;
                    g->assert_expr(m.mk_or(m.mk_not(info.m_bvar), cnstr), 0, 0);
                }
                return info.m_bvar;
            }
        }

        expr * mk_aux_lit(cnstr2bvar & c2b, expr * cnstr, bool produce_models, goal_ref const & g) {
            bool sign = false;
            while (m.is_not(cnstr)) {
                cnstr = to_app(cnstr)->get_arg(0);
                sign  = !sign;
            }
            
            SASSERT(!c2b.contains(cnstr));
            expr * bvar = m.mk_fresh_const(0, m.mk_bool_sort());
            if (produce_models)
                m_mc->insert(to_app(bvar)->get_decl());
            c2b.insert(cnstr, bvar_info(bvar, sign));
            if (sign) {
                g->assert_expr(m.mk_or(bvar, m.mk_not(cnstr)), 0, 0);
                return m.mk_not(bvar);
            }
            else {
                g->assert_expr(m.mk_or(m.mk_not(bvar), cnstr), 0, 0);
                return bvar;
            }
        }
        
        void operator()(goal_ref const & g, 
                        goal_ref_buffer & result, 
                        model_converter_ref & mc, 
                        proof_converter_ref & pc,
                        expr_dependency_ref & core) {
            SASSERT(g->is_well_sorted());
            mc = 0; pc = 0; core = 0;

            fail_if_proof_generation("occf", g);

            bool produce_models = g->models_enabled();
            tactic_report report("occf", *g);
            
            m_mc = 0;
            
            ptr_vector<expr> new_lits;
            
            cnstr2bvar c2b;
            
            unsigned sz = g->size();
            for (unsigned i = 0; i < sz; i++) {
                checkpoint();
                expr * f = g->form(i);
                expr_dependency * d = g->dep(i);
                if (!m.is_or(f))
                    continue;
                app * cls = to_app(f);
                if (!is_target(cls))
                    continue;
                if (produce_models && !m_mc) {
                    m_mc = alloc(filter_model_converter, m);
                    mc = m_mc;
                }
                expr * keep = 0;
                new_lits.reset();
                unsigned num = cls->get_num_args();
                for (unsigned j = 0; j < num; j++) {
                    expr * l = cls->get_arg(j);
                    if (is_constraint(l)) {
                        expr * new_l = get_aux_lit(c2b, l, g);
                        if (new_l != 0) {
                            new_lits.push_back(new_l);
                        }
                        else if (keep == 0) {
                            keep = l;
                        }
                        else {
                            new_l = mk_aux_lit(c2b, l, produce_models, g);
                            new_lits.push_back(new_l);
                        }
                    }
                    else {
                        new_lits.push_back(l);
                    }
                }
                if (keep != 0)
                    new_lits.push_back(keep);
                g->update(i, m.mk_or(new_lits.size(), new_lits.c_ptr()), 0, d);
            }
            g->inc_depth();
            result.push_back(g.get());
            TRACE("occf", g->display(tout););
            SASSERT(g->is_well_sorted());
        }
    };
    
    imp *      m_imp;
public:
    occf_tactic(ast_manager & m) {
        m_imp = alloc(imp, m);
    }

    virtual tactic * translate(ast_manager & m) {
        return alloc(occf_tactic, m);
    }
        
    virtual ~occf_tactic() {
        dealloc(m_imp);
    }

    virtual void updt_params(params_ref const & p) {}
    virtual void collect_param_descrs(param_descrs & r) {}
    
    virtual void operator()(goal_ref const & in, 
                            goal_ref_buffer & result, 
                            model_converter_ref & mc, 
                            proof_converter_ref & pc,
                            expr_dependency_ref & core) {
        (*m_imp)(in, result, mc, pc, core);
    }
    
    virtual void cleanup() {
        imp * d = alloc(imp, m_imp->m);
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

tactic * mk_occf_tactic(ast_manager & m, params_ref const & p) {
    return clean(alloc(occf_tactic, m));
}

