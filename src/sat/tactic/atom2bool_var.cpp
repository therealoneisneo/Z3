/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    atom2bool_var.cpp

Abstract:

    The mapping between SAT boolean variables and atoms

Author:

    Leonardo (leonardo) 2011-10-25

Notes:

--*/
#include"atom2bool_var.h"
#include"ast_smt2_pp.h"
#include"ref_util.h"
#include"goal.h"

void atom2bool_var::mk_inv(expr_ref_vector & lit2expr) const {
    obj_map<expr, var>::iterator it  = m_mapping.begin();
    obj_map<expr, var>::iterator end = m_mapping.end();
    for (; it != end; ++it) {
        sat::literal l(static_cast<sat::bool_var>(it->m_value), false);
        lit2expr.set(l.index(), it->m_key);
        l.neg();
        lit2expr.set(l.index(), m().mk_not(it->m_key));
    }
}

sat::bool_var atom2bool_var::to_bool_var(expr * n) const {
    sat::bool_var v = sat::null_bool_var;
    m_mapping.find(n, v);
    return v;
}

struct collect_boolean_interface_proc {
    struct visitor {
        obj_hashtable<expr> & m_r;
        visitor(obj_hashtable<expr> & r):m_r(r) {}
        void operator()(var * n)  {}
        void operator()(app * n)  { if (is_uninterp_const(n)) m_r.insert(n); }       
        void operator()(quantifier * n) {} 
    };

    ast_manager &    m;
    expr_fast_mark2  fvisited;
    expr_fast_mark1  tvisited;
    ptr_vector<expr> todo;
    visitor          proc;

    collect_boolean_interface_proc(ast_manager & _m, obj_hashtable<expr> & r):
        m(_m),
        proc(r) {
    }

    void process(expr * f) {
        if (fvisited.is_marked(f))
            return;
        fvisited.mark(f);
        todo.push_back(f);
        while (!todo.empty()) {
            expr * t = todo.back();
            todo.pop_back();
            if (is_uninterp_const(t))
                continue;
            if (is_app(t) && to_app(t)->get_family_id() == m.get_basic_family_id() && to_app(t)->get_num_args() > 0) {
                decl_kind k = to_app(t)->get_decl_kind();
                if (k == OP_OR || k == OP_NOT || k == OP_IFF || ((k == OP_EQ || k == OP_ITE) && m.is_bool(to_app(t)->get_arg(1)))) {
                    unsigned num = to_app(t)->get_num_args();
                    for (unsigned i = 0; i < num; i++) {
                        expr * arg = to_app(t)->get_arg(i);
                        if (fvisited.is_marked(arg))
                            continue;
                        fvisited.mark(arg);
                        todo.push_back(arg);
                    }
                }
            }
            else {
                quick_for_each_expr(proc, tvisited, t);
            }
        }
    }
    
    template<typename T>
    void operator()(T const & g) {
        unsigned sz = g.size();
        for (unsigned i = 0; i < sz; i++)
            process(g.form(i));
    }
    
    void operator()(unsigned sz, expr * const * fs) {
        for (unsigned i = 0; i < sz; i++)
            process(fs[i]);
    }
};

template<typename T>
void collect_boolean_interface_core(T const & s, obj_hashtable<expr> & r) {
    collect_boolean_interface_proc proc(s.m(), r);
    proc(s);
}

void collect_boolean_interface(goal const & g, obj_hashtable<expr> & r) {
    collect_boolean_interface_core(g, r);
}

void collect_boolean_interface(ast_manager & m, unsigned num, expr * const * fs, obj_hashtable<expr> & r) {
    collect_boolean_interface_proc proc(m, r);
    proc(num, fs);
}
