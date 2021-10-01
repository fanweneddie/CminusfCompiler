#ifndef SYSYC_VALUE_H
#define SYSYC_VALUE_H

#include <string>
#include <list>
#include <iostream>

class Type;
class Value;

// a class that represents a use case
struct Use
{
    // the current value
    Value *val_;
    // the no. of operand, e.g., func(a, b), a is 0, b is 1
    unsigned arg_no_;
    Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}
};

// a class that represents value in Cminus
// just like Value in Jimple
class Value
{
public:
    explicit Value(Type *ty, const std::string &name = "");
    ~Value() = default;

    Type *get_type() const { return type_; }

    std::list<Use> &get_use_list() { return use_list_; }

    void add_use(Value *val, unsigned arg_no = 0);

    bool set_name(std::string name) { 
        if (name_ == "")
        {
            name_=name;
            return true;
        }   
        return false; 
    }
    std::string get_name() const;

    virtual std::string print() = 0;
private:
    // the type of the value
    Type *type_;
    // a list of use case that shows which value uses this value
    std::list<Use> use_list_;
    // the name of this value
    std::string name_;
};

#endif // SYSYC_VALUE_H
