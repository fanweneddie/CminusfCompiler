#ifndef SYSYC_FUNCTION_H
#define SYSYC_FUNCTION_H

#include <iterator>
#include <list>
#include <cstddef>
#include <map>
#include <cassert>

#include "User.h"
#include "Module.h"
#include "BasicBlock.h"
#include "Type.h"

class Module;
class Argument;
class BasicBlock;
class Type;
class FunctionType;

// a class that represents function in Cminus
class Function : public Value
{
public:
    Function(FunctionType *ty, const std::string &name, Module *parent);
    ~Function();
    static Function *create(FunctionType *ty, const std::string &name, Module *parent);

    FunctionType *get_function_type() const;

    Type *get_return_type() const;

    void add_basic_block(BasicBlock *bb);

    unsigned get_num_of_args() const;
    
    Module *get_parent() const;

    std::list<Argument *>::iterator arg_begin() { return arguments_.begin(); }
    std::list<Argument *>::iterator arg_end() { return arguments_.end(); }
    
    void remove(BasicBlock* bb) { basic_blocks_.remove(bb); }
    std::list<BasicBlock *> get_basic_blocks() { return basic_blocks_; }
    std::list<Argument *> get_args() { return arguments_; }
    
    bool is_declaration() { return basic_blocks_.empty(); }
    
    void set_instr_name();
    std::string print();

private:
    void build_args();

private:
    // basic blocks
    std::list<BasicBlock *> basic_blocks_;
    // arguments
    std::list<Argument *> arguments_;
    // the module that this function is defined in
    Module *parent_;

};

// Argument of Function, does not contain actual value
class Argument : public Value
{
public:
    /// Argument constructor.
    explicit Argument(Type *ty, const std::string &name = "", Function *f = nullptr,
                    unsigned arg_no = 0)
        : Value(ty, name), parent_(f), arg_no_(arg_no) {}
    ~Argument() {}

    inline const Function *get_parent() const { return parent_; }
    inline       Function *get_parent()       { return parent_; }

    /// For example in "void foo(int a, float b)" a is 0 and b is 1.
    unsigned get_arg_no() const {
        assert(parent_ && "can't get number of unparented arg");
        return arg_no_;
    }

    virtual std::string print() override ;
private:
    // the function that this argument is in
    Function *parent_;
    // argument No.
    unsigned arg_no_; 
};

#endif // SYSYC_FUNCTION_H
