#include "cminusf_builder.hpp"
// I write this code for fun.
// I have referenced to the official solution. 
// writing this lab is really addictive, just can't stop it ... 
// chengli, yyds!!! 

/// type constants for llvm IR
IntegerType* INT_1_TYPE;
IntegerType* INT_32_TYPE;
FloatType* FLOAT_32_TYPE;
Type* LABEL_TYPE;
Type* VOID_TYPE;
PointerType* INT_32_PTR_TYPE;
PointerType* FLOAT_32_PTR_TYPE;

/// global variables
// current function.
// when we are outside of any function, cur_func = null
Function* cur_func = nullptr;
// current value
Value* cur_val = nullptr;
// current arguments of function
std::vector<Argument*> args;
// the index of current argument when visiting param
int arg_index = 0;
// whether the variable is a left value
bool is_left_val = false;

/// Transfrom a Cminus type to IR type. 
/// Cminus type can only be int, float or void. 
/// Otherwise, the returned type will be nullptr 
/// @param cminus_type      a Cminus type
/// @param is_array         whether it is an array type
/// @return                 the corresponding IR type 
Type* CminusType_to_IRType(CminusType cminus_type, bool is_array);

/// Check whether the operator is an int operator. 
/// Also, we can transform int to float if needed.
/// @param builder          IR builder to build transform instructions
/// @param l_val            Points to left value
/// @param r_val            Points to right value
/// @return                 true iff the operator is an int operator 
bool Is_int_operation(IRBuilder* builder, Value** l_val, Value** r_val);

/// Visit a node of ASTProgram. 
/// We should first get the type constants above. 
/// Then, we need to visit its declarations for further building. 
/// @param node     a node of ASTProgram 
void CminusfBuilder::visit(ASTProgram &node) {
    
    // get type constants from module
    INT_1_TYPE = module->get_int1_type();
    INT_32_TYPE = module->get_int32_type();
    FLOAT_32_TYPE = module->get_float_type();
    LABEL_TYPE = module->get_label_type();
    VOID_TYPE = module->get_void_type();
    INT_32_PTR_TYPE = module->get_int32_ptr_type();
    FLOAT_32_PTR_TYPE = module->get_float_ptr_type();

    // continue visiting every declaration
    for (auto decl : node.declarations) {
        decl->accept(*this);
    }
}

/// Visit a node of ASTNum. 
/// We need to store the value into cur_val for later use. 
/// @param node     a node of ASTNum
void CminusfBuilder::visit(ASTNum &node) {
    
    // sanity check
    assert((node.type == TYPE_INT || node.type == TYPE_FLOAT) 
        && "a number can only be int or float");

    // store the value into cur_val
    if (node.type == TYPE_INT) {
        cur_val = ConstantInt::get(node.i_val, module.get());
    }
    else {
        cur_val = ConstantFP::get(node.f_val, module.get());
    }
}

/// Visit a node of ASTVarDeclaration. 
/// If it is a global declaration, we should init the variable as 0. 
/// If it is a local declaration, we should allocate space for the variable. 
/// Finally, we should add the variable into scope(as a symbol table). 
/// @param node     a node of ASTDeclaration 
void CminusfBuilder::visit(ASTVarDeclaration &node) {
    
    // get the type of this variable
    Type* var_type = CminusType_to_IRType(node.type, false);
    assert((var_type == INT_32_TYPE || var_type == FLOAT_32_TYPE) 
            && "variable type can only be int and float");

    // check whether this variable is an array
    // 1. the variable is an array
    if (node.num) {
        // get the type of array
        auto array_type = ArrayType::get(var_type, node.num.get()->i_val);
        // check whether the declaration is global or local
        // 1. global, init a global array with 0s and push this variable into scope
        if (scope.in_global()) {
            auto const_zero = ConstantZero::get(array_type, module.get());
            auto global_var = GlobalVariable::create(node.id, module.get(), 
                                array_type, false, const_zero);
            scope.push(node.id, global_var);
        }
        // 2. local, allocate space in stack and push the variable into scope
        else {
            auto local_var = builder->create_alloca(array_type);
            scope.push(node.id, local_var);
        }
    }
    // 2. the variable is not an array
    else {
        // check whether the declaration is global or local
        // 1. global, init a global var with 0 and push this variable into scope
        if (scope.in_global()) {
            auto const_zero = ConstantZero::get(var_type, module.get());
            auto global_var = GlobalVariable::create(node.id, module.get(), 
                                var_type, false, const_zero);
            scope.push(node.id, global_var);
        }
        // 2. local, allocate space in stack and push the variable into scope
        else {
            auto local_var = builder->create_alloca(var_type);
            scope.push(node.id, local_var);
        }
    }
}

/// Visit a node of ASTFuncDeclaration. 
/// First, we get its signature(types of return value and parameters)
/// and create a function in IR. 
/// Then, we create a new basic block and enter a new scope. 
/// Finally, we continue exploring its parameters and function body. 
/// @param node     a node of ASTFuncDeclaration 
void CminusfBuilder::visit(ASTFunDeclaration &node) {

    /// ----------- to prepare for this function -----------

    // save the value of current function
    Function* old_func = cur_func;

    // a list that stores the type of parameters of this function
    std::vector<Type *> params;
    // get params by transforming each parameter type
    for (auto param : node.params) {
        Type* param_type = CminusType_to_IRType(param->type, param->isarray);
        assert(param_type != nullptr 
            && "parameter type should only be int, float or void.");
        params.push_back(param_type);
    }
    
    // get return type of this function
    Type* return_type = CminusType_to_IRType(node.type, false);
    assert(return_type && "return type should only be int, float or void.");

    // get the type of function
    auto func_type = FunctionType::get(return_type, params);

    // create this function in IR, push it into scope
    auto func = Function::create(func_type, node.id, module.get());
    scope.push(node.id, func);

    /// ----------- to explore inside the function -----------

    // enter the scope of this function
    cur_func = func;

    // create a new basic block for this function and insert it into builder
    auto new_bb = BasicBlock::create(module.get(), node.id, cur_func);
    auto cur_bb = builder->get_insert_block();
    if (cur_bb) {
        cur_bb->add_succ_basic_block(new_bb);
    }
    new_bb->add_pre_basic_block(cur_bb);
    builder->set_insert_point(new_bb);

    // get the arguments of this function
    args.clear();
    for (auto arg_itr = func->arg_begin(); arg_itr != func->arg_end(); ++arg_itr) {
        args.push_back(*arg_itr);
    }

    // continue visiting the parameters
    arg_index = 0;
    for (auto param : node.params) {
        param->accept(*this);
        arg_index++;
    }

    // continue visiting the function body
    node.compound_stmt->accept(*this);

    // add ret instruction if the function doesn't have a return statement
    cur_bb = builder->get_insert_block();
    if (!cur_bb->get_terminator()) {
        // check the return type of this function
        // 1. return void
        if (func->get_return_type()->is_void_type()) {
            builder->create_void_ret();
        } 
        // 2. return int
        else if (func->get_return_type()->is_integer_type()) {
            builder->create_ret(ConstantInt::get(0, module.get()));
        } 
        // 3. return float
        else if (func->get_return_type()->is_float_type()) {
            builder->create_ret(ConstantFP::get(0.0, module.get()));
        }
    }

    /// ----------- exit from the function -----------

    // restore cur_func
    cur_func = old_func;
}

/// Visit a node of ASTParam. 
/// Here, we allocate space for the parameter, 
/// and store the value of arguments into the space
/// @param node     a node of ASTParam 
void CminusfBuilder::visit(ASTParam &node) {

    // get the type of this parameter
    Type* param_type = CminusType_to_IRType(node.type, false);
    assert(param_type != nullptr 
        && "parameter type should only be int, float or void.");

    // we don't need to do anything if the parameter is void
    if (param_type == VOID_TYPE)
        return;

    // the register to be allocated
    Value* alloc_reg = nullptr;
    // check whether this parameter is an array
    // 1. the parameter is an array
    if (node.isarray) {
        // allocate this array, based on element type
        if (param_type == INT_32_TYPE) {
            alloc_reg = builder->create_alloca(INT_32_PTR_TYPE);
        } else {
            alloc_reg = builder->create_alloca(FLOAT_32_PTR_TYPE);
        }
    }
    // 2. the parameter is not an array
    else {
        // allocate this variable
        if (param_type == INT_32_TYPE) {
            alloc_reg = builder->create_alloca(INT_32_TYPE);
        } else {
            alloc_reg = builder->create_alloca(FLOAT_32_TYPE);
        }
    }
    // store the argument into the allocated register
    builder->create_store(args[arg_index], alloc_reg);
    scope.push(node.id, alloc_reg);
}

/// Visit a node of ASTCompoundStmt. 
/// Here we just need to continue exploring local declarations and statements
/// @param node     a node of ASTCompoundStmt 
void CminusfBuilder::visit(ASTCompoundStmt &node) {

    scope.enter();
    for (auto local_decl : node.local_declarations) {
        local_decl->accept(*this);
    }

    for (auto stmt : node.statement_list) {
        stmt->accept(*this);
    }
    scope.exit();
}

/// Visit a node of ASTExpressionStmt. 
/// Here we just need to continue exploring the (possible)ASTExpresion node. 
/// If this expression is simply ";", then we do nothing. 
/// @param node     a node of ASTExpressionStmt
void CminusfBuilder::visit(ASTExpressionStmt &node) {
    
    if (node.expression) {
        node.expression->accept(*this);
    }
}

/// Visit a node of ASTSelectionStmt. 
/// We need to continue visiting the conditional expression,
/// if-statement and else-statement
/// @param node     a node of ASTExpressionStmt
void CminusfBuilder::visit(ASTSelectionStmt &node) {

    // get the current basic block
    auto cur_bb = builder->get_insert_block();
    // create corresponding basic blocks: for true branch, false branch and next block
    auto true_bb = BasicBlock::create(module.get(), "", cur_func);
    auto false_bb = BasicBlock::create(module.get(), "", cur_func);
    auto next_bb = BasicBlock::create(module.get(), "", cur_func);

    // get the result of expression in cur_val
    node.expression->accept(*this);
    
    // get the result of proper compare instruction
    Value* cmp_result = nullptr;
    if (cur_val->get_type()->is_integer_type()) {
        cmp_result = builder->create_icmp_ne(cur_val, 
                        ConstantInt::get(0, module.get()));
    } else {
        cmp_result = builder->create_fcmp_ne(cur_val, 
                        ConstantFP::get(0.0, module.get()));
    }
    
    // generate the branch instruction and connect basic blocks
    // 1. there exists else statement
    if (node.else_statement) {
        builder->create_cond_br(cmp_result, true_bb, false_bb);
        // connect basic blocks
        cur_bb->add_succ_basic_block(true_bb);
        true_bb->add_pre_basic_block(cur_bb);
        cur_bb->add_succ_basic_block(false_bb);
        false_bb->add_pre_basic_block(cur_bb);
    }
    // 2. there does not exist else statement
    else {
        builder->create_cond_br(cmp_result, true_bb, next_bb);
        // remove false_bb since it is not used
        false_bb->erase_from_parent();
        // connect basic blocks
        cur_bb->add_succ_basic_block(true_bb);
        true_bb->add_pre_basic_block(cur_bb);
        cur_bb->add_succ_basic_block(next_bb);
        next_bb->add_pre_basic_block(cur_bb);
    }
    
    // continue visiting true branch
    builder->set_insert_point(true_bb);
    node.if_statement->accept(*this);
    // connect basic blocks
    auto cur_nested_bb = builder->get_insert_block();
    if (!cur_nested_bb->get_terminator()) {
        cur_nested_bb->add_succ_basic_block(next_bb);
        next_bb->add_pre_basic_block(cur_nested_bb);
        builder->create_br(next_bb);
    }

    // continue visiting false branch(if exists)
    if (node.else_statement) {
        builder->set_insert_point(false_bb);
        node.else_statement->accept(*this);
        // connect basic blocks
        cur_nested_bb = builder->get_insert_block();
        if (!cur_nested_bb->get_terminator()) {
            cur_nested_bb->add_succ_basic_block(next_bb);
            next_bb->add_pre_basic_block(cur_nested_bb);
            builder->create_br(next_bb);
        }
    }
    builder->set_insert_point(next_bb);
}

/// Visit a node of ASTIterationStmt
/// We just need to explore the conditional expression and
/// statements in iteration
/// @param node     a node of ASTIterationStmt
void CminusfBuilder::visit(ASTIterationStmt &node) {
    
    // get the current basic block
    auto cur_bb = builder->get_insert_block();
    // create corresponding basic blocks: for comparion, true branch and next block
    auto cmp_bb = BasicBlock::create(module.get(), "", cur_func);
    auto true_bb = BasicBlock::create(module.get(), "", cur_func);
    auto next_bb = BasicBlock::create(module.get(), "", cur_func);

    // get the result of expression in cur_val in cmp_bb
    cur_bb->add_succ_basic_block(cmp_bb);
    cmp_bb->add_pre_basic_block(cur_bb);
    builder->create_br(cmp_bb);
    builder->set_insert_point(cmp_bb);
    node.expression->accept(*this);

    // get the result of proper compare instruction
    Value* cmp_result = nullptr;
    if (cur_val->get_type()->is_integer_type()) {
        cmp_result = builder->create_icmp_ne(cur_val, 
                        ConstantInt::get(0, module.get()));
    } else {
        cmp_result = builder->create_fcmp_ne(cur_val, 
                        ConstantFP::get(0.0, module.get()));
    }

    // generate the branch instruction and connect basic blocks
    builder->create_cond_br(cmp_result, true_bb, next_bb);
    cmp_bb->add_succ_basic_block(true_bb);
    true_bb->add_pre_basic_block(cmp_bb);
    cmp_bb->add_succ_basic_block(next_bb);
    next_bb->add_pre_basic_block(cmp_bb);

    // continue visiting statements in iteration
    builder->set_insert_point(true_bb);
    node.statement->accept(*this);
    // connect basic blocks
    auto cur_nested_bb = builder->get_insert_block();
    if (!cur_nested_bb->get_terminator()) {
        builder->create_br(cmp_bb);
        cur_nested_bb->add_succ_basic_block(cmp_bb);
        cmp_bb->add_pre_basic_block(cur_nested_bb);
    }

    // focus on next_bb for next instructions
    builder->set_insert_point(next_bb);
}


/// Visit a node of ASTReturnStmt
/// We just need to explore the returned expression and
/// do type transformation
/// (we don't need to connect basic blocks inter-procedurally)
/// @param node     a node of ASTReturnStmt
void CminusfBuilder::visit(ASTReturnStmt &node) {

    // check whether it is a void return
    // 1. void return
    if (!node.expression) {
        builder->create_void_ret();
    } 
    // 2. return a value
    else {
        // get the result of returned expression in cur_val
        node.expression->accept(*this);
        // do type transformation
        auto ret_expr_type = cur_val->get_type();
        auto func_ret_type = cur_func->get_return_type();
        if (ret_expr_type != func_ret_type) {
            // 1. integer to float
            if (func_ret_type->is_float_type()) {
                cur_val = builder->create_sitofp(cur_val, FLOAT_32_TYPE);
            } 
            // 2. float to integer
            else {
                cur_val = builder->create_fptosi(cur_val, INT_32_TYPE);
            }
        }
        // generate ret instruction
        builder->create_ret(cur_val);
    }
}

/// Visit a node of ASTVar.
/// @param node     a node of ASTVar
void CminusfBuilder::visit(ASTVar &node) {

    // get the value of the variable and do sanity check
    auto value = scope.find(node.id);
    assert(value != nullptr && "the variable should be declared");
    // get the flag of the type of the value and do sanity check
    auto value_type = value->get_type()->get_pointer_element_type();
    bool is_array = value_type->is_array_type();
    if (is_array) {
        value_type = static_cast<ArrayType *>(value_type)->get_element_type();
    }
    bool is_int = value_type->is_integer_type();
    bool is_float = value_type->is_float_type();
    bool is_ptr = value_type->is_pointer_type();
    bool cur_is_left_val = is_left_val;

    // check whether the variable is an array
    // 1. the expression does not access an array
    if (!node.expression) {
        // check whether this variable is a left value
        // 1. this variable is a left value, just return its address
        if (cur_is_left_val) {
            cur_val = value;
            is_left_val = false;
        }
        // 2. this variable is not a left value
        // load it or dereference it
        else {
            if (is_array) {
                cur_val = builder->create_gep(value, {ConstantInt::get(0, module.get())});
            } else if (is_int || is_float || is_ptr) {
                cur_val = builder->create_load(value);
            } else {
                cur_val = builder->create_gep(value, {ConstantInt::get(0, module.get())});
            }
        }
    } 
    // 2. the variable accesses an array
    else {
        // continue visiting the expression
        is_left_val = false;
        node.expression->accept(*this);
        auto expr_val = cur_val;
        if (expr_val->get_type()->is_float_type()) {
            expr_val = builder->create_fptosi(expr_val, INT_32_TYPE);
        }

        // get the element
        if (is_int || is_float) {
            cur_val = builder->create_gep(value, {ConstantInt::get(0, module.get()), expr_val});
        } else if (is_ptr) {
            cur_val = builder->create_load(value);
            cur_val = builder->create_gep(cur_val, {expr_val});
        } else {
            cur_val = builder->create_gep(value, {ConstantInt::get(0, module.get())});
        }

        // if the array reference is not a left value,
        // just load the array element into a register
        if (!cur_is_left_val) {
            cur_val = builder->create_load(cur_val);
        }
    }
}

/// Visit a node of ASTAssignExpression. 
/// We need to get the expression result and address. 
/// Then we can do type transformation and store the value
/// @param node     a node of ASTAssignExpression
void CminusfBuilder::visit(ASTAssignExpression &node) {

    // visit the right expression and the left value
    node.expression->accept(*this);
    auto expr_result = cur_val;
    is_left_val = true;
    node.var->accept(*this);
    auto addr = cur_val;
    
    // do type transform(if possible) for right value
    auto l_type = addr->get_type()->get_pointer_element_type();
    if (l_type->is_array_type()) {
        l_type = static_cast<ArrayType *>(l_type)->get_element_type();
    }
    auto r_type = expr_result->get_type();
    
    if (l_type != r_type) {
        // int to float
        if (r_type == INT_32_TYPE) {
            expr_result = builder->create_sitofp(expr_result, FLOAT_32_TYPE);
        // float to int
        } else if (r_type == FLOAT_32_TYPE) {
            expr_result = builder->create_fptosi(expr_result, INT_32_TYPE);
        }
    }

    // store and update cur_val
    builder->create_store(expr_result, addr);
    cur_val = expr_result;
}

/// Visit a node of ASTSimpleExpression.
/// We need to do type transform if possible. 
/// @param node     a node of ASTSimpleExpression
void CminusfBuilder::visit(ASTSimpleExpression &node) {

    // check the orgnization of the simple expression
    // 1. just an additive expression
    if (!node.additive_expression_r) {
        node.additive_expression_l->accept(*this);
    }
    // 2. two additive expressions linked by a relational operator
    else {
        // get left and right value
        node.additive_expression_l->accept(*this);
        auto l_val = cur_val;
        node.additive_expression_r->accept(*this);
        auto r_val = cur_val;

        // do type transformation and get the operation type
        bool is_int = Is_int_operation(builder, &l_val, &r_val);
        // the result of operation
        Value* result;
        // check the opration type and generate instruction
        switch(node.op) {
            // <=
            case OP_LE:
                if (is_int) {
                    result = builder->create_icmp_le(l_val, r_val);
                } else {
                    result = builder->create_fcmp_le(l_val, r_val);
                }
                break;
            // <
            case OP_LT:
                if (is_int) {
                    result = builder->create_icmp_lt(l_val, r_val);
                } else {
                    result = builder->create_fcmp_lt(l_val, r_val);
                }
                break;
            // >
            case OP_GT:
                if (is_int) {
                    result = builder->create_icmp_gt(l_val, r_val);
                } else {
                    result = builder->create_fcmp_gt(l_val, r_val);
                }
                break;
            // >=
            case OP_GE:
                if (is_int) {
                    result = builder->create_icmp_ge(l_val, r_val);
                } else {
                    result = builder->create_fcmp_ge(l_val, r_val);
                }
                break;
            // ==
            case OP_EQ:
                if (is_int) {
                    result = builder->create_icmp_eq(l_val, r_val);
                } else {
                    result = builder->create_fcmp_eq(l_val, r_val);
                }
                break;
            // !=
            case OP_NEQ:
                if (is_int) {
                    result = builder->create_icmp_ne(l_val, r_val);
                } else {
                    result = builder->create_fcmp_ne(l_val, r_val);
                }
                break;
            default:
                result = nullptr;
                assert(nullptr && "strange relational operator type");
                break;
        }
        // extend the result from bool to int32, and save it in cur_val
        cur_val = builder->create_zext(result, INT_32_TYPE);
    }
}

/// Visit a node of ASTAdditiveExpression.
/// We need to do type transform if possible. 
/// @param node     a node of ASTAdditiveExpression
void CminusfBuilder::visit(ASTAdditiveExpression &node) {

    // check the organization of the additive expression
    // 1. the additive expression is only made up of term
    if (!node.additive_expression) {
        node.term->accept(*this);
    }
    // 2. the additive expression is linked by additive operator
    // similar to the case in visiting ASTSimpleExpression
    else {
        node.additive_expression->accept(*this);
        auto l_val = cur_val;
        node.term->accept(*this);
        auto r_val = cur_val;
        bool is_int = Is_int_operation(builder, &l_val, &r_val);

        // get the register to store the result, and save it in cur_val
        Value* result;
        switch(node.op) {
            // +
            case OP_PLUS:
                if (is_int) {
                    result = builder->create_iadd(l_val, r_val);
                } else {
                    result = builder->create_fadd(l_val, r_val);
                }
                break;
            // -
            case OP_MINUS:
                if (is_int) {
                    result = builder->create_isub(l_val, r_val);
                } else {
                    result = builder->create_fsub(l_val, r_val);
                }
                break;
            default:
                result = nullptr;
                assert( nullptr && "strange additive operator type");
                break;
        }
        cur_val = result;
    }
}

/// Visit a node of ASTTerm
/// We need to do type transform if possible. 
/// @param node     a node of ASTTerm
void CminusfBuilder::visit(ASTTerm &node) {

    // check the organization of the term
    // 1. the term is only made up of factor
    if (!node.term) {
        node.factor->accept(*this);
    }
    // 2. the term is linked by a multiply operator
    // similar to the case in visiting ASTSimpleExpression
    else {
        node.term->accept(*this);
        auto l_val = cur_val;
        node.factor->accept(*this);
        auto r_val = cur_val;
        bool is_int = Is_int_operation(builder, &l_val, &r_val);

        Value* result;
        switch(node.op) {
            // *
            case OP_MUL:
                if (is_int) {
                    result = builder->create_imul(l_val, r_val);
                } else {
                    result = builder->create_fmul(l_val, r_val);
                }
                break;
            // /
            case OP_DIV:
                if (is_int) {
                    result = builder->create_isdiv(l_val, r_val);
                } else {
                    result = builder->create_fdiv(l_val, r_val);
                }
                break;
            default:
                result = nullptr;
                assert( nullptr && "strange multiply operator type");
                break;
        }

        // save result into cur_val
        cur_val = result;
    }
}

/// Visit a node of ASTCall
/// We need to do type transform if possible. 
/// @param node     a node of ASTCall
void CminusfBuilder::visit(ASTCall &node) {

    // get the calling function and do sanity check
    auto func = static_cast<Function *>(scope.find(node.id));
    assert(func && "function should be declared first");
    assert(func->get_function_type()->get_num_of_args() == node.args.size()
            && "number of formal argument should be equal to number of actual argument");

    // the list of actual arguments
    std::vector<Value*> actual_args;
    // the iterator of formal argument type 
    auto formal_arg_type_itr = func->get_function_type()->param_begin();

    // visit the arguments(do type transformation if possible),
    // and then get the list of actual arguments
    for (auto arg : node.args) {
        arg->accept(*this);
        // type transformation for argument
        if (cur_val->get_type() != *formal_arg_type_itr) {
            // 1. float <-> int
            if (!(*formal_arg_type_itr)->is_pointer_type()) {
                if ((*formal_arg_type_itr)->is_integer_type()) {
                    cur_val = builder->create_fptosi(cur_val, INT_32_TYPE);
                } else {
                    cur_val = builder->create_sitofp(cur_val, FLOAT_32_TYPE);
                }
            }
            // 2. array -> pointer
            else {
                cur_val = builder->create_gep(cur_val, {ConstantInt::get(0, module.get()),
                                                        ConstantInt::get(0, module.get())});
            }
        }
        actual_args.push_back(cur_val);
        formal_arg_type_itr++;
    }

    // generate a call instruction
    cur_val = builder->create_call(func, actual_args);
}

Type* CminusType_to_IRType(CminusType cminus_type, bool is_array) {

    Type* IR_type;
    switch (cminus_type) {
        case TYPE_INT:
            if (is_array) {
                IR_type = INT_32_PTR_TYPE;
            } else {
                IR_type = INT_32_TYPE;
            }
            break;
        case TYPE_FLOAT:
            if (is_array) {
                IR_type = FLOAT_32_PTR_TYPE;
            } else {
                IR_type = FLOAT_32_TYPE;
            }
            break;
        case TYPE_VOID:
            IR_type = VOID_TYPE;
            break;
        default:
            IR_type = nullptr;
            break;
    }

    return IR_type;
}

bool Is_int_operation(IRBuilder* builder, Value** l_val, Value **r_val) {
    
    // marks whether the operator is int operator
    bool is_int;
    auto l_type = (*l_val)->get_type();
    auto r_type = (*r_val)->get_type();

    // check the type of left/right value and do type transformation
    // 1. both int, no need to transform
    if (l_type->is_integer_type() && r_type->is_integer_type()){
        is_int = true;
    }
    // 2. not both int, so transform int value to float
    else {
        is_int = false;
        if (l_type->is_integer_type()) {
            *l_val = builder->create_sitofp(*l_val, FLOAT_32_TYPE);
        }
        if (r_type->is_integer_type()) {
            *r_val = builder->create_sitofp(*r_val, FLOAT_32_TYPE);
        }
    }

    return is_int;
}