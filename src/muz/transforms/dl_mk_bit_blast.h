/*++
Copyright (c) 2012 Microsoft Corporation

Module Name:

    dl_mk_bit_blast.h

Abstract:

    Functor for bit-blasting a rule set

Author:

    Nikolaj Bjorner (nbjorner) 2012-08-30

Revision History:

--*/
#ifndef _DL_MK_BIT_BLAST_H_
#define _DL_MK_BIT_BLAST_H_

#include"dl_rule_transformer.h"

namespace datalog {

    class mk_bit_blast : public rule_transformer::plugin {
        class impl;
        impl* m_impl;
    public:
        mk_bit_blast(context & ctx, unsigned priority = 35000);
        ~mk_bit_blast();        
        rule_set * operator()(rule_set const & source);
    };

};

#endif /* _DL_MK_BIT_BLAST_H_ */

