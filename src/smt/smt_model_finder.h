/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    smt_model_finder.h

Abstract:

    Model finding goodies for universally quantified formulas.
    
    During the search, the finder store information about the quantifiers
    that are internalized. In an ideal world, quantifiers are only internalized
    at base level.

    Given a satisfiable ground formula, Z3 will restrict the interpretation
    of uninterpreted functions in a finite subset of its domain.
    The model finder tries to produce a complete interpretation that will
    also satisfy all universally quantified formulas.

    During model construction, the model finder will complete the interpretation
    of uninterpreted functions by propagating basic constraints induced by the
    body of universally quantified formulas.
    
    More information can be found in the following papers:

    - Complete instantiation for quantified SMT formulas, Yeting Ge
      and Leonardo de Moura, Conference on Computer Aided Verification
      (CAV 2009), Grenoble, France, 2009.

    - Efficiently Solving Quantified Bit-Vector Formula, Christoph
      Wintersteiger, Youssef Hamadi and Leonardo de Moura, FMCAD,
      Lugano, Switzerland, 2010.

    - Bugs, Moles and Skeletons: Symbolic Reasoning for Software
      Development, Leonardo de Moura, Nikolaj Bjorner, IJCAR,
      Edinburgh, Scotland, 2010.

Author:

    Leonardo de Moura (leonardo) 2010-12-17.

Revision History:

--*/
#ifndef _SMT_MODEL_FINDER_H_
#define _SMT_MODEL_FINDER_H_

#include"ast.h"
#include"func_decl_dependencies.h"
#include"simplifier.h"
#include"proto_model.h"

namespace smt {
    class context;
    
    namespace mf {
        class quantifier_info;
        class quantifier_analyzer;
        class auf_solver;
        class simple_macro_solver;
        class hint_solver;
        class non_auf_macro_solver;
        class instantiation_set;
    };
        
    class model_finder {
        typedef mf::quantifier_analyzer        quantifier_analyzer;
        typedef mf::quantifier_info            quantifier_info;
        typedef mf::auf_solver                 auf_solver;
        typedef mf::simple_macro_solver        simple_macro_solver;
        typedef mf::hint_solver                hint_solver;
        typedef mf::non_auf_macro_solver       non_auf_macro_solver;
        typedef mf::instantiation_set          instantiation_set;

        ast_manager &                          m_manager;
        context *                              m_context;
        scoped_ptr<quantifier_analyzer>        m_analyzer;
        scoped_ptr<auf_solver>                 m_auf_solver;
        obj_map<quantifier, quantifier_info *> m_q2info;
        ptr_vector<quantifier>                 m_quantifiers;
        func_decl_dependencies                 m_dependencies;
        scoped_ptr<simple_macro_solver>        m_sm_solver;
        scoped_ptr<hint_solver>                m_hint_solver;
        scoped_ptr<non_auf_macro_solver>       m_nm_solver;
        
        struct scope {
            unsigned                           m_quantifiers_lim;
        };
        
        svector<scope>                         m_scopes;
        
        expr_ref_vector                        m_new_constraints; // new constraints for fresh constants created by the model finder

        void restore_quantifiers(unsigned old_size);
        quantifier_info * get_quantifier_info(quantifier * q) const;
        void collect_relevant_quantifiers(ptr_vector<quantifier> & qs) const;
        void cleanup_quantifier_infos(ptr_vector<quantifier> const & qs);
        void process_simple_macros(ptr_vector<quantifier> & qs, ptr_vector<quantifier> & residue, proto_model * m);
        void process_hint_macros(ptr_vector<quantifier> & qs, ptr_vector<quantifier> & residue, proto_model * m);
        void process_non_auf_macros(ptr_vector<quantifier> & qs, ptr_vector<quantifier> & residue, proto_model * m);
        void process_auf(ptr_vector<quantifier> const & qs, proto_model * m);
        instantiation_set const * get_uvar_inst_set(quantifier * q, unsigned i) const;

    public:
        model_finder(ast_manager & m, simplifier & s);
        ~model_finder();
        void set_context(context * ctx);
        
        void register_quantifier(quantifier * q);
        void push_scope();
        void pop_scope(unsigned num_scopes);
        void reset();
        void init_search_eh();
        void fix_model(proto_model * m);

        quantifier * get_flat_quantifier(quantifier * q) const;
        expr * get_inv(quantifier * q, unsigned i, expr * val, unsigned & generation) const;
        bool restrict_sks_to_inst_set(context * aux_ctx, quantifier * q, expr_ref_vector const & sks);

        void restart_eh();
    };
};

#endif
