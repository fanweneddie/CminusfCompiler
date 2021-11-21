#ifndef SYSYC_USER_H
#define SYSYC_USER_H

#include "Value.h"
#include <vector>

// the class of a user of a value
// it can actually be an instruction,
// (if we say an instruction uses a value)
class User : public Value
{
public:
    User(Type *ty, const std::string &name = "", unsigned num_ops = 0);
    ~User() = default;

    std::vector<Value *>& get_operands();
 
    Value *get_operand(unsigned i) const;

    void set_operand(unsigned i, Value *v);

    unsigned get_num_operand() const;
private:
    // the value that this user uses
    std::vector<Value *> operands_;
    // number of operands
    unsigned num_ops_;
};

#endif // SYSYC_USER_H
