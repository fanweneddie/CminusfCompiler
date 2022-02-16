#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP

#include "Module.h"
#include "Function.h"
#include "PassManager.hpp"

/*
 * Do constant propagation pass for a function.
 * Lattice: product lattice of maps of each variable
 * Direction: forward
 * Top element: UNDEF (undefined to be a constant)
 * Bottom element: NAC (definitely not a constant)
 * Meet operator: "Intersect"
 * Transfer function: given a statement s
 *      if s is not an assignment statement, then f is an identity function
 *      if s is an assignment statement for x, then for v != x, f(m(v)) = m(v)
 *          for x, if RHS is a constant c, then f(m(x))(x) = c
 *                 if RHS is y op z, then
 *                      if m(y) and m(z) are const, then f(m(x))(x) = m(y) + m(z)
 *                      if either m(y) or m(z) is NAC, then f(m(x))(x) = NAC
 *                      else, f(m(x))(x) = UNDEF
 */
class ConstPropagation : public Pass {
public:
    ConstPropagation(Module *m) : Pass(m) {}
    ~ConstPropagation() {};
    // Do constant propagation in a function, since this is an intra-procedural analysis
    // @param func: the function to be analyzed
    void process_func(Function *func);
    void run() override;
private:
};

/*
 * The state of a constant value, which is the element in the lattice
 */
class ConstState {
public:
    // shows whether the value is undefined/absolutely not/absolutely a constant
    enum const_type {
        UNDEF,  // undefined to be a constant
        NAC,    // absolutely not a constant
        MBC     // must be a constant
    };

    // the constant value
    Constant *const_value;
};

#endif