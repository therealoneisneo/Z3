/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    rational.h

Abstract:

    Rational numbers

Author:

    Leonardo de Moura (leonardo) 2006-09-18.

Revision History:

--*/
#ifndef _RATIONAL_H_
#define _RATIONAL_H_

#include"mpq.h"

class rational {
    mpq   m_val;
    static rational                  m_zero;
    static rational                  m_one;
    static rational                  m_minus_one;
    static vector<rational>          m_powers_of_two;
    static synch_mpq_manager *       g_mpq_manager;
    
    static synch_mpq_manager & m() { return *g_mpq_manager; }

public:
    static void initialize();
    static void finalize();
    /*
      ADD_INITIALIZER('rational::initialize();')
      ADD_FINALIZER('rational::finalize();')
    */
    rational() {}
    
    rational(rational const & r) { m().set(m_val, r.m_val); }

    explicit rational(int n) { m().set(m_val, n); }

    explicit rational(unsigned n) { m().set(m_val, n); }
      
    rational(int n, int d) { m().set(m_val, n, d); }

    rational(mpq const & q) { m().set(m_val, q); }

    rational(mpz const & z) { m().set(m_val, z); }

    explicit rational(char const * v) { m().set(m_val, v); }

    struct i64 {};
    rational(int64 i, i64) { m().set(m_val, i); }

    struct ui64 {};
    rational(uint64 i, ui64) { m().set(m_val, i); }
    
    ~rational() { m().del(m_val); }
    
    mpq const & to_mpq() const { return m_val; }

    unsigned bitsize() const { return m().bitsize(m_val); }
    
    void reset() { m().reset(m_val); }

    bool is_int() const { return m().is_int(m_val); }

    bool is_small() const { return m().is_small(m_val); }

    bool is_big() const { return !is_small(); }
    
    unsigned hash() const { return m().hash(m_val); }

    struct hash_proc {  unsigned operator()(rational const& r) const { return r.hash(); }  };

    struct eq_proc { bool operator()(rational const& r1, rational const& r2) const { return r1 == r2; } };
    
    void swap(rational & n) { m().swap(m_val, n.m_val); }
    
    std::string to_string() const { return m().to_string(m_val); }

    void display(std::ostream & out) const { return m().display(out, m_val); }
    
    void display_decimal(std::ostream & out, unsigned prec) const { return m().display_decimal(out, m_val, prec); }

    bool is_uint64() const { return m().is_uint64(m_val); }

    bool is_int64() const { return m().is_int64(m_val); }

    uint64 get_uint64() const { return m().get_uint64(m_val); }

    int64 get_int64() const { return m().get_int64(m_val); }
    
    bool is_unsigned() const { return is_uint64() && (get_uint64() < (1ull << 32)); }

    unsigned get_unsigned() const {
        SASSERT(is_unsigned());
        return static_cast<unsigned>(get_uint64());
    }

    bool is_int32() const {
        if (is_small() && is_int()) return true; 
        // we don't assume that if it is small, then it is int32.
        if (!is_int64()) return false;
        int64 v = get_int64();
        return INT_MIN <= v && v <= INT_MAX;
    }

    double get_double() const { return m().get_double(m_val); }

    rational const & get_rational() const { return *this; }

    rational const & get_infinitesimal() const { return m_zero; }
    
    rational & operator=(rational const & r) {
        m().set(m_val, r.m_val);
        return *this;
    }

    friend inline rational numerator(rational const & r) { rational result; m().get_numerator(r.m_val, result.m_val); return result; }
    
    friend inline rational denominator(rational const & r) { rational result; m().get_denominator(r.m_val, result.m_val); return result; }
    
    rational & operator+=(rational const & r) { 
        m().add(m_val, r.m_val, m_val);
        return *this; 
    }

    rational & operator-=(rational const & r) { 
        m().sub(m_val, r.m_val, m_val);
        return *this; 
    }

    rational & operator*=(rational const & r) {
        m().mul(m_val, r.m_val, m_val);
        return *this; 
    }    

    rational & operator/=(rational const & r) {
        m().div(m_val, r.m_val, m_val);
        return *this; 
    }    
    
    rational & operator%=(rational const & r) {
        m().rem(m_val, r.m_val, m_val);
        return *this; 
    }    

    friend inline rational div(rational const & r1, rational const & r2) {
        rational r;
        rational::m().idiv(r1.m_val, r2.m_val, r.m_val);
        return r;
    }

    friend inline void div(rational const & r1, rational const & r2, rational & r) {
        rational::m().idiv(r1.m_val, r2.m_val, r.m_val);
    }
    
    friend inline rational machine_div(rational const & r1, rational const & r2) {
        rational r;
        rational::m().machine_idiv(r1.m_val, r2.m_val, r.m_val);
        return r;
    }

    friend inline rational mod(rational const & r1, rational const & r2) {
        rational r;
        rational::m().mod(r1.m_val, r2.m_val, r.m_val);
        return r;
    }

    friend inline void mod(rational const & r1, rational const & r2, rational & r) {
        rational::m().mod(r1.m_val, r2.m_val, r.m_val);
    }

    friend inline rational operator%(rational const & r1, rational const & r2) {
        rational r;
        rational::m().rem(r1.m_val, r2.m_val, r.m_val);
        return r;
    }

    friend inline rational mod_hat(rational const & a, rational const & b) {
        SASSERT(b.is_pos());
        rational r = mod(a,b);
        SASSERT(r.is_nonneg());
        rational r2 = r;
        r2 *= rational(2);
        if (operator<(b, r2)) {
            r -= b;
        }
        return r;
    }

    rational & operator++() {
        m().add(m_val, m().mk_q(1), m_val);
        return *this;
    }

    const rational operator++(int) { rational tmp(*this); ++(*this); return tmp; }
    
    rational & operator--() {
        m().sub(m_val, m().mk_q(1), m_val);
        return *this;
    }
    
    const rational operator--(int) { rational tmp(*this); --(*this); return tmp; }

    friend inline bool operator==(rational const & r1, rational const & r2) {
        return rational::m().eq(r1.m_val, r2.m_val);
    }

    friend inline bool operator<(rational const & r1, rational const & r2) { 
        return rational::m().lt(r1.m_val, r2.m_val);
    }
    
    void neg() {
        m().neg(m_val);
    }

    bool is_zero() const {
        return m().is_zero(m_val);
    }

    bool is_one() const {
        return m().is_one(m_val);
    }

    bool is_minus_one() const {
        return m().is_minus_one(m_val);
    }

    bool is_neg() const {
        return m().is_neg(m_val);
    }
    
    bool is_pos() const {
        return m().is_pos(m_val);
    }
    
    bool is_nonneg() const {
        return m().is_nonneg(m_val);
    }
    
    bool is_nonpos() const {
        return m().is_nonpos(m_val);
    }

    bool is_even() const {
        return m().is_even(m_val);
    }
    
    friend inline rational floor(rational const & r) {
        rational f;
        rational::m().floor(r.m_val, f.m_val);
        return f;
    }

    friend inline rational ceil(rational const & r) {
        rational f;
        rational::m().ceil(r.m_val, f.m_val);
        return f;
    }

    rational expt(int n) const {
        rational result;
        m().power(m_val, n, result.m_val);
        return result;
    }

    static rational power_of_two(unsigned k);

    bool is_power_of_two(unsigned & shift) {
        return m().is_power_of_two(m_val, shift);
    }

    static rational const & zero() {
        return m_zero;
    }

    static rational const & one() {
        return m_one;
    }

    static rational const & minus_one() {
        return m_minus_one;
    }

    void addmul(rational const & c, rational const & k) {
        if (c.is_one())
            operator+=(k);
        else if (c.is_minus_one())
            operator-=(k);
        else {
            rational tmp(k);
            tmp *= c;
            operator+=(tmp);
        }
    }

    // Perform:  this -= c * k
    void submul(const rational & c, const rational & k) {
        if (c.is_one())
            operator-=(k);
        else if (c.is_minus_one())
            operator+=(k);
        else {
            rational tmp(k);
            tmp *= c;
            operator-=(tmp);
        }
    }

    bool is_int_perfect_square(rational & root) const {
        return m().is_int_perfect_square(m_val, root.m_val);
    }

    bool is_perfect_square(rational & root) const {
        return m().is_perfect_square(m_val, root.m_val);
    }

    bool root(unsigned n, rational & root) const {
        return m().root(m_val, n, root.m_val);
    }

    friend inline std::ostream & operator<<(std::ostream & target, rational const & r) {
        target << m().to_string(r.m_val);
        return target;
    }

    friend inline rational gcd(rational const & r1, rational const & r2);

    //
    // extended Euclid:
    // r1*a + r2*b = gcd
    //
    friend inline rational gcd(rational const & r1, rational const & r2, rational & a, rational & b);

    friend inline rational lcm(rational const & r1, rational const & r2) {
        rational result;
        m().lcm(r1.m_val, r2.m_val, result.m_val);
        return result;
    }

    friend inline rational bitwise_or(rational const & r1, rational const & r2) {
        rational result;
        m().bitwise_or(r1.m_val, r2.m_val, result.m_val);
        return result;
    }

    friend inline rational bitwise_and(rational const & r1, rational const & r2) {
        rational result;
        m().bitwise_and(r1.m_val, r2.m_val, result.m_val);
        return result;
    }

    friend inline rational bitwise_xor(rational const & r1, rational const & r2) {
        rational result;
        m().bitwise_xor(r1.m_val, r2.m_val, result.m_val);
        return result;
    }

    friend inline rational bitwise_not(unsigned sz, rational const & r1) {
        rational result;
        m().bitwise_not(sz, r1.m_val, result.m_val);
        return result;
    }

    friend inline rational abs(rational const & r);

    rational to_rational() const { return *this; }

    static bool is_rational() { return true; }

    unsigned get_num_bits() const {
        rational two(2);
        SASSERT(is_int());
        SASSERT(!is_neg());
        rational n(*this);
        unsigned num_bits = 1;
        n = div(n, two);
        while (n.is_pos()) {
            ++num_bits;
            n = div(n, two);
        }
        return num_bits;
    }

};

inline bool operator!=(rational const & r1, rational const & r2) { 
    return !operator==(r1, r2); 
}

inline bool operator>(rational const & r1, rational const & r2) { 
    return operator<(r2, r1); 
}

inline bool operator<=(rational const & r1, rational const & r2) { 
    return !operator>(r1, r2); 
}

inline bool operator>=(rational const & r1, rational const & r2) { 
    return !operator<(r1, r2); 
}

inline rational operator+(rational const & r1, rational const & r2) { 
    return rational(r1) += r2; 
}

inline rational operator-(rational const & r1, rational const & r2) { 
    return rational(r1) -= r2; 
}

inline rational operator-(rational const & r) { 
    rational result(r); 
    result.neg(); 
    return result; 
}

inline rational operator*(rational const & r1, rational const & r2) { 
    return rational(r1) *= r2; 
}

inline rational operator/(rational const & r1, rational const & r2) { 
    return rational(r1) /= r2; 
}

inline rational power(rational const & r, unsigned p) {
    return r.expt(p);
}

inline rational abs(rational const & r) {
  rational result(r);
  rational::m().abs(result.m_val);
  return result;
}

inline rational gcd(rational const & r1, rational const & r2) {
  rational result;
  rational::m().gcd(r1.m_val, r2.m_val, result.m_val);
  return result;
}

inline rational gcd(rational const & r1, rational const & r2, rational & a, rational & b) {
  rational result;
  rational::m().gcd(r1.m_val, r2.m_val, a.m_val, b.m_val, result.m_val);
  return result;
}


#endif /* _RATIONAL_H_ */

