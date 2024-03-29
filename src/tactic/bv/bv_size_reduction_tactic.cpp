/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    bv_size_reduction_tactic.cpp

Abstract:

    Reduce the number of bits used to encode constants, by using signed bounds.
    Example: suppose x is a bit-vector of size 8, and we have
    signed bounds for x such that:
        -2 <= x <= 2
    Then, x can be replaced by  ((sign-extend 5) k)
    where k is a fresh bit-vector constant of size 3.

Author:

    Leonardo (leonardo) 2012-02-19

Notes:

--*/
#include"tactical.h"
#include"bv_decl_plugin.h"
#include"expr_replacer.h"
#include"extension_model_converter.h"
#include"filter_model_converter.h"
#include"ast_smt2_pp.h"

class bv_size_reduction_tactic : public tactic {
    struct imp;
    imp *      m_imp;
public:
    bv_size_reduction_tactic(ast_manager & m);

    virtual tactic * translate(ast_manager & m) {
        return alloc(bv_size_reduction_tactic, m);
    }

    virtual ~bv_size_reduction_tactic();

    virtual void operator()(goal_ref const & g, goal_ref_buffer & result, model_converter_ref & mc, proof_converter_ref & pc, expr_dependency_ref & core);

    virtual void cleanup();
    virtual void set_cancel(bool f);
};

tactic * mk_bv_size_reduction_tactic(ast_manager & m, params_ref const & p) {
    return clean(alloc(bv_size_reduction_tactic, m));
}

struct bv_size_reduction_tactic::imp {
    typedef rational numeral;
    typedef extension_model_converter bv_size_reduction_mc;
    
    ast_manager &             m;
    bv_util                   m_util;
    obj_map<app, numeral>     m_signed_lowers;
    obj_map<app, numeral>     m_signed_uppers;
    obj_map<app, numeral>     m_unsigned_lowers;
    obj_map<app, numeral>     m_unsigned_uppers;
    ref<bv_size_reduction_mc> m_mc;
    ref<filter_model_converter> m_fmc;
    scoped_ptr<expr_replacer> m_replacer;
    bool                      m_produce_models;
    volatile bool             m_cancel;

    imp(ast_manager & _m):
        m(_m),
        m_util(m),
        m_replacer(mk_default_expr_replacer(m)),
        m_cancel(false) {
    }

    void update_signed_lower(app * v, numeral const & k) {
        // k <= v
        obj_map<app, numeral>::obj_map_entry * entry = m_signed_lowers.insert_if_not_there2(v, k);
        if (entry->get_data().m_value < k) {
            // improve bound
            entry->get_data().m_value = k;
        }
    }

    void update_signed_upper(app * v, numeral const & k) {
        // v <= k
        obj_map<app, numeral>::obj_map_entry * entry = m_signed_uppers.insert_if_not_there2(v, k);
        if (k < entry->get_data().m_value) {
            // improve bound
            entry->get_data().m_value = k;
        }
    }
    
    void update_unsigned_lower(app * v, numeral const & k) {
        SASSERT(k > numeral(0));
        // k <= v
        obj_map<app, numeral>::obj_map_entry * entry = m_unsigned_lowers.insert_if_not_there2(v, k);
        if (entry->get_data().m_value < k) {
            // improve bound
            entry->get_data().m_value = k;
        }
    }

    void update_unsigned_upper(app * v, numeral const & k) {
        SASSERT(k > numeral(0));
        // v <= k
        obj_map<app, numeral>::obj_map_entry * entry = m_unsigned_uppers.insert_if_not_there2(v, k);
        if (k < entry->get_data().m_value) {
            // improve bound
            entry->get_data().m_value = k;
        }
    }

    void collect_bounds(goal const & g) {
        unsigned sz = g.size();
        numeral  val;
        unsigned bv_sz;
        expr * f, * lhs, * rhs;        
        for (unsigned i = 0; i < sz; i++) {
            bool negated = false;
            f = g.form(i);            
            if (m.is_not(f)) {
                negated = true;
                f = to_app(f)->get_arg(0);
            }

            if (m_util.is_bv_sle(f, lhs, rhs)) {
                bv_sz = m_util.get_bv_size(lhs);
                if (is_uninterp_const(lhs) && m_util.is_numeral(rhs, val, bv_sz)) {
                    TRACE("bv_size_reduction", tout << (negated?"not ":"") << mk_ismt2_pp(f, m) << std::endl; );
                    // v <= k
                    val = m_util.norm(val, bv_sz, true);
                    if (negated) {
                        val += numeral(1);
                        if (m_util.norm(val, bv_sz, true) != val) {
                            // bound is infeasible.
                        } 
                        else {
                            update_signed_lower(to_app(lhs), val);
                        }
                    }
                    else update_signed_upper(to_app(lhs), val);
                }
                else if (is_uninterp_const(rhs) && m_util.is_numeral(lhs, val, bv_sz)) {
                    TRACE("bv_size_reduction", tout << (negated?"not ":"") << mk_ismt2_pp(f, m) << std::endl; );
                    // k <= v
                    val = m_util.norm(val, bv_sz, true);
                    if (negated) {
                        val -= numeral(1);
                        if (m_util.norm(val, bv_sz, true) != val) {
                            // bound is infeasible.
                        } 
                        else {
                            update_signed_upper(to_app(lhs), val);
                        }                        
                    }
                    else update_signed_lower(to_app(rhs), val);
                }
            }
            
#if 0
            else if (m_util.is_bv_ule(f, lhs, rhs)) {
                if (is_uninterp_const(lhs) && m_util.is_numeral(rhs, val, bv_sz)) {
                    TRACE("bv_size_reduction", tout << (negated?"not ":"") << mk_ismt2_pp(f, m) << std::endl; );
                    // v <= k
                    if (negated) update_unsigned_lower(to_app(lhs), val+numeral(1));
                    else update_unsigned_upper(to_app(lhs), val);
                }
                else if (is_uninterp_const(rhs) && m_util.is_numeral(lhs, val, bv_sz)) {
                    TRACE("bv_size_reduction", tout << (negated?"not ":"") << mk_ismt2_pp(f, m) << std::endl; );
                    // k <= v
                    if (negated) update_unsigned_upper(to_app(rhs), val-numeral(1));
                    else update_unsigned_lower(to_app(rhs), val);                    
                }
            }
#endif
        }
    }
    
    void checkpoint() {
        if (m_cancel)
            throw tactic_exception(TACTIC_CANCELED_MSG);
    }
    
    void operator()(goal & g, model_converter_ref & mc) {
        if (g.inconsistent())
            return;
        TRACE("before_bv_size_reduction", g.display(tout););
        m_produce_models = g.models_enabled();
        mc = 0;
        m_mc = 0;
        unsigned num_reduced = 0;
        {
            tactic_report report("bv-size-reduction", g);
            collect_bounds(g);
            
            // create substitution
            expr_substitution subst(m);

            if (!(m_signed_lowers.empty() || m_signed_uppers.empty())) {
                TRACE("bv_size_reduction", 
                        tout << "m_signed_lowers: " << std::endl;
                        for (obj_map<app, numeral>::iterator it = m_signed_lowers.begin(); it != m_signed_lowers.end(); it++)
                            tout << mk_ismt2_pp(it->m_key, m) << " >= " << it->m_value.to_string() << std::endl;
                        tout << "m_signed_uppers: " << std::endl;
                        for (obj_map<app, numeral>::iterator it = m_signed_uppers.begin(); it != m_signed_uppers.end(); it++)
                            tout << mk_ismt2_pp(it->m_key, m) << " <= " << it->m_value.to_string() << std::endl;
                        );                    
                
                obj_map<app, numeral>::iterator it  = m_signed_lowers.begin();
                obj_map<app, numeral>::iterator end = m_signed_lowers.end();
                for (; it != end; ++it) {
                    app * v = it->m_key;
                    unsigned bv_sz = m_util.get_bv_size(v);
                    numeral l = m_util.norm(it->m_value, bv_sz, true);
                    obj_map<app, numeral>::obj_map_entry * entry = m_signed_uppers.find_core(v);
                    if (entry != 0) {
                        numeral u = m_util.norm(entry->get_data().m_value, bv_sz, true);
                        TRACE("bv_size_reduction", tout << l << " <= " << v->get_decl()->get_name() << " <= " << u << "\n";);
                        expr * new_def = 0;
                        app  * new_const = 0;
                        if (l > u) {
                            g.assert_expr(m.mk_false());
                            return;
                        }
                        else if (l == u) {
                            new_def = m_util.mk_numeral(l, m.get_sort(v));
                        }
                        else {
                            // l < u
                            if (l.is_neg()) {
                                unsigned i_nb = (u - l).get_num_bits();
                                unsigned v_nb = m_util.get_bv_size(v);
                                if (i_nb < v_nb) {
                                    new_const = m.mk_fresh_const(0, m_util.mk_sort(i_nb));
                                    new_def = m_util.mk_sign_extend(v_nb - i_nb, new_const);
                                }
                            }
                            else {
                                // 0 <= l <= v <= u
                                unsigned u_nb = u.get_num_bits();
                                unsigned v_nb = m_util.get_bv_size(v);
                                if (u_nb < v_nb) {
                                    new_const = m.mk_fresh_const(0, m_util.mk_sort(u_nb));
                                    new_def = m_util.mk_concat(m_util.mk_numeral(numeral(0), v_nb - u_nb), new_const);
                                }
                            }
                        }
                    
                        if (new_def) {
                            subst.insert(v, new_def);
                            if (m_produce_models) {
                                if (!m_mc) 
                                    m_mc = alloc(bv_size_reduction_mc, m);
                                m_mc->insert(v->get_decl(), new_def);
                                if (!m_fmc && new_const) 
                                    m_fmc = alloc(filter_model_converter, m);
                                if (new_const) 
                                    m_fmc->insert(new_const->get_decl());
                            }
                            num_reduced++;
                        }
                    }
                }
            }
            
#if 0            
            if (!(m_unsigned_lowers.empty() && m_unsigned_uppers.empty())) {
                TRACE("bv_size_reduction", 
                    tout << "m_unsigned_lowers: " << std::endl;
                    for (obj_map<app, numeral>::iterator it = m_unsigned_lowers.begin(); it != m_unsigned_lowers.end(); it++)
                        tout << mk_ismt2_pp(it->m_key, m) << " >= " << it->m_value.to_string() << std::endl;
                    tout << "m_unsigned_uppers: " << std::endl;
                    for (obj_map<app, numeral>::iterator it = m_unsigned_uppers.begin(); it != m_unsigned_uppers.end(); it++)
                        tout << mk_ismt2_pp(it->m_key, m) << " <= " << it->m_value.to_string() << std::endl;
                    );

                obj_map<app, numeral>::iterator it  = m_unsigned_uppers.begin();
                obj_map<app, numeral>::iterator end = m_unsigned_uppers.end();
                for (; it != end; ++it) {
                    app * v = it->m_key;
                    unsigned bv_sz = m_util.get_bv_size(v);
                    numeral u = m_util.norm(it->m_value, bv_sz, false);
                    obj_map<app, numeral>::obj_map_entry * entry = m_signed_lowers.find_core(v);                    
                    numeral l = (entry != 0) ? m_util.norm(entry->get_data().m_value, bv_sz, false) : numeral(0);
                                        
                    obj_map<app, numeral>::obj_map_entry * lse = m_signed_lowers.find_core(v);
                    obj_map<app, numeral>::obj_map_entry * use = m_signed_uppers.find_core(v);
                    if ((lse != 0 && lse->get_data().m_value > l) &&
                        (use != 0 && use->get_data().m_value < u))
                        continue; // Skip, we had better signed bounds.

                    if (lse != 0 && lse->get_data().m_value > l) l = lse->get_data().m_value;                    
                    if (use != 0 && use->get_data().m_value < u) u = use->get_data().m_value;

                    TRACE("bv_size_reduction", tout << l << " <= " << v->get_decl()->get_name() << " <= " << u << "\n";);
                    expr * new_def = 0;
                    app * new_const = 0;
                    if (l > u) {
                        g.assert_expr(m.mk_false());
                        return;
                    }
                    else if (l == u) {
                        new_def = m_util.mk_numeral(l, m.get_sort(v));
                    }
                    else {
                        // 0 <= l <= v <= u
                        unsigned u_nb = u.get_num_bits();
                        unsigned v_nb = m_util.get_bv_size(v);
                        if (u_nb < v_nb) {
                            new_def = m_util.mk_concat(m_util.mk_numeral(numeral(0), v_nb - u_nb), new_const);
                            new_const = m.mk_fresh_const(0, m_util.mk_sort(u_nb));
                        }
                    }
                    
                    if (new_def) {
                        subst.insert(v, new_def);
                        if (m_produce_models) {
                            if (!m_mc) 
                                m_mc = alloc(bv_size_reduction_mc, m);
                            m_mc->insert(v->get_decl(), new_def);
                            if (!m_fmc && new_const) 
                                m_fmc = alloc(filter_model_converter, m);
                            if (new_const) 
                                m_fmc->insert(new_const->get_decl());
                        }
                        num_reduced++;
                        TRACE("bv_size_reduction", tout << "New definition = " << mk_ismt2_pp(new_def, m) << "\n";);
                    }
                }
            }
#endif

            if (subst.empty())
                return;
            
            m_replacer->set_substitution(&subst);
            
            unsigned sz = g.size();
            expr * f;
            expr_ref new_f(m);
            for (unsigned i = 0; i < sz; i++) {
                if (g.inconsistent())
                    return;
                f = g.form(i);
                (*m_replacer)(f, new_f);
                g.update(i, new_f);
            }
            mc   = m_mc.get();
            if (m_fmc) {
                mc = concat(m_fmc.get(), mc.get());
            }
            m_mc = 0;
            m_fmc = 0;
        }
        report_tactic_progress(":bv-reduced", num_reduced);
        TRACE("after_bv_size_reduction", g.display(tout); if (m_mc) m_mc->display(tout););
    }

    void set_cancel(bool f) {
        m_replacer->set_cancel(f);
        m_cancel = f;
    }
};

bv_size_reduction_tactic::bv_size_reduction_tactic(ast_manager & m) {
    m_imp = alloc(imp, m);
}

bv_size_reduction_tactic::~bv_size_reduction_tactic() {
    dealloc(m_imp);
}

void bv_size_reduction_tactic::operator()(goal_ref const & g, 
                                          goal_ref_buffer & result, 
                                          model_converter_ref & mc, 
                                          proof_converter_ref & pc,
                                          expr_dependency_ref & core) {
    SASSERT(g->is_well_sorted());
    fail_if_proof_generation("bv-size-reduction", g);
    fail_if_unsat_core_generation("bv-size-reduction", g);
    mc = 0; pc = 0; core = 0; result.reset();
    m_imp->operator()(*(g.get()), mc);
    g->inc_depth();
    result.push_back(g.get());
    SASSERT(g->is_well_sorted());
}

void bv_size_reduction_tactic::set_cancel(bool f) {
    if (m_imp)
        m_imp->set_cancel(f);
}
 
void bv_size_reduction_tactic::cleanup() {
    imp * d = alloc(imp, m_imp->m);
    #pragma omp critical (tactic_cancel)
    {
        std::swap(d, m_imp);
    }
    dealloc(d);
}

