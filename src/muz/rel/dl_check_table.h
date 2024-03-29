/*++
Copyright (c) 2010 Microsoft Corporation

Module Name:

    dl_check_table.h

Abstract:

    <abstract>

Author:

    Nikolaj Bjorner (nbjorner) 2010-11-15
    

Revision History:

--*/

#ifndef _DL_CHECK_TABLE_H_
#define _DL_CHECK_TABLE_H_

#include "dl_base.h"
#include "dl_decl_plugin.h"
#include "dl_relation_manager.h"

namespace datalog {
    class check_table;

    class check_table_plugin : public table_plugin {
        friend class check_table;
        table_plugin& m_checker;
        table_plugin& m_tocheck;
        unsigned m_count;
    protected:
        class join_fn;
        class join_project_fn;
        class union_fn;
        class transformer_fn;
        class rename_fn;
        class project_fn;
        class select_equal_and_project_fn;
        class filter_equal_fn;
        class filter_identical_fn;
        class filter_interpreted_fn;
        class filter_interpreted_and_project_fn;
        class filter_by_negation_fn;

    public:
        check_table_plugin(relation_manager & manager, symbol const& checker, symbol const& tocheck) 
            : table_plugin(symbol("check"), manager),
            m_checker(*manager.get_table_plugin(checker)),
            m_tocheck(*manager.get_table_plugin(tocheck)), m_count(0) {}

        virtual table_base * mk_empty(const table_signature & s);

        virtual table_join_fn * mk_join_fn(const table_base & t1, const table_base & t2,
            unsigned col_cnt, const unsigned * cols1, const unsigned * cols2);
        virtual table_join_fn * mk_join_project_fn(const table_base & t1, const table_base & t2,
            unsigned col_cnt, const unsigned * cols1, const unsigned * cols2, unsigned removed_col_cnt, 
            const unsigned * removed_cols);
        virtual table_union_fn * mk_union_fn(const table_base & tgt, const table_base & src, 
            const table_base * delta);
        virtual table_transformer_fn * mk_project_fn(const table_base & t, unsigned col_cnt, 
            const unsigned * removed_cols);
        virtual table_transformer_fn * mk_select_equal_and_project_fn(const table_base & t, 
            const table_element & value, unsigned col);
        virtual table_transformer_fn * mk_rename_fn(const table_base & t, unsigned permutation_cycle_len,
            const unsigned * permutation_cycle);
        virtual table_mutator_fn * mk_filter_identical_fn(const table_base & t, unsigned col_cnt, 
            const unsigned * identical_cols);
        virtual table_mutator_fn * mk_filter_equal_fn(const table_base & t, const table_element & value, 
            unsigned col);
        virtual table_mutator_fn * mk_filter_interpreted_fn(const table_base & t, app * condition);
        virtual table_transformer_fn * mk_filter_interpreted_and_project_fn(const table_base & t,
            app * condition, unsigned removed_col_cnt, const unsigned * removed_cols);
        virtual table_intersection_filter_fn * mk_filter_by_negation_fn(
            const table_base & t, 
            const table_base & negated_obj, unsigned joined_col_cnt, 
            const unsigned * t_cols, const unsigned * negated_cols);

        virtual bool can_handle_signature(table_signature const& s);

    private:
        static check_table& get(table_base& r);

        static check_table const & get(table_base const& r);  

        static table_base& checker(table_base& r);
        static table_base const& checker(table_base const& r);
        static table_base* checker(table_base* r);
        static table_base const* checker(table_base const* r);
        static table_base& tocheck(table_base& r);
        static table_base const& tocheck(table_base const& r);
        static table_base* tocheck(table_base* r);
        static table_base const* tocheck(table_base const* r);
    };

    class check_table : public table_base {
        friend class check_table_plugin;

        table_base* m_checker;
        table_base* m_tocheck;
            
        check_table(check_table_plugin & p, const table_signature & sig);
        check_table(check_table_plugin & p, const table_signature & sig, table_base* tocheck, table_base* checker);

        virtual ~check_table();

        bool well_formed() const;

    public:

        check_table_plugin & get_plugin() const { 
            return static_cast<check_table_plugin &>(table_base::get_plugin()); 
        }

        virtual bool empty() const;
        virtual void add_fact(const table_fact & f);
        virtual void remove_fact(const table_element*  fact);
        virtual bool contains_fact(const table_fact & f) const;       
        virtual table_base * complement(func_decl* p, const table_element * func_columns = 0) const;
        virtual table_base * clone() const;

        virtual iterator begin() const { SASSERT(well_formed()); return m_tocheck->begin(); }
        virtual iterator end() const { return m_tocheck->end(); }

        virtual unsigned get_size_estimate_rows() const { return m_tocheck->get_size_estimate_rows(); }
        virtual unsigned get_size_estimate_bytes() const { return m_tocheck->get_size_estimate_bytes(); }
    };

 };

 #endif /* _DL_CHECK_TABLE_H_ */
