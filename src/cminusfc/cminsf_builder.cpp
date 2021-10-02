#include "cminusf_builder.hpp"
// I have referenced to the official solution. 
// writing this lab is really addictive, just can't stop it ... 
// chengli, yyds!!! 

/// type constants

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

/// Transfrom a Cminus type to IR type. 
/// Cminus type can only be int, float or void. 
/// Otherwise, the returned type will be nullptr 
/// @param cminus_type      a Cminus type 
/// @return                 the corresponding IR type 
Type* CminusType_to_IRType(CminusType cminus_type);

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

    // continue visit every declaration
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
    Type* var_type = CminusType_to_IRType(node.type);
    assert( (var_type == INT_32_TYPE || var_type == FLOAT_32_TYPE) 
            && "variable type can only be int and float");

    // check whether this variable is an array
    // 1. array
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
    // 2. not array
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
        Type* param_type = CminusType_to_IRType(param->type);
        assert(param_type && "parameter type should only be int, float or void.");
        params.push_back(param_type);
    }
    
    // get return type of this function
    Type* return_type = CminusType_to_IRType(node.type);
    assert(return_type && "return type should only be int, float or void.");

    // get the type of function
    auto func_type = FunctionType::get(return_type, params);

    // create this function in IR, push it into scope
    auto func = Function::create(func_type, node.id, module.get());
    scope.push(node.id, func);

    /// ----------- to explore inside the function -----------

    // enter the scope of this function
    cur_func = func;
    scope.enter();

    // create a new basic block for this function and insert it into builder
    auto new_bb = BasicBlock::create(module.get(), node.id, cur_func);
    auto cur_bb = builder->get_insert_block();
    cur_bb->add_succ_basic_block(new_bb);
    new_bb->add_pre_basic_block(cur_bb);
    builder->set_insert_point(new_bb);

    // continue visiting the parameters
    for (auto param : node.params) {
        param->accept(*this);
    }

    // continue visiting the function body
    node.compound_stmt->accept(*this);

    /// ----------- exit from the function -----------

    // restore the scope and cur_func
    scope.exit();
    cur_func = old_func;
}

/// Visit a node of ASTParam. 
/// Here, we allocate space for the parameter. 
/// @param node     a node of ASTParam 
/// ?????????????????????????? how to allocate???
void CminusfBuilder::visit(ASTParam &node) {

    // get the type of this parameter
    Type* param_type = CminusType_to_IRType(node.type);
    assert(param_type && "parameter type should only be int, float or void.");

    // we don't need to do anything if the parameter is void
    if (param_type == VOID_TYPE)
        return;

    // check whether this parameter is an array
    // 1. array
    if (node.isarray) {
        // allocate this array
        if (param_type == INT_32_TYPE) {

        } else {

        }
    }
    // 2. not array
    else {
        // allocate this parameter
        if (param_type == INT_32_TYPE) {

        } else {

        }
    }
}

/// Visit a node of ASTCompoundStmt. 
/// Here we just need to continue exploring local declarations and statements
/// @param node     a node of ASTCompoundStmt 
void CminusfBuilder::visit(ASTCompoundStmt &node) {

    for (auto local_decl : node.local_declarations) {
        local_decl->accept(*this);
    }
    for (auto stmt : node.statement_list) {
        stmt->accept(*this);
    }
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
/// We need to continue visiting the conditional expression. 
/// @param node     a node of ASTExpressionStmt
void CminusfBuilder::visit(ASTSelectionStmt &node) {

    node.expression->accept(*this);
    
    // create corresponding basic blocks: for true branch, false branch and next
    auto true_bb = BasicBlock::create(module.get(), "", cur_func);
    auto false_bb = BasicBlock::create(module.get(), "", cur_func);
    auto next_bb = BasicBlock::create(module.get(), "", cur_func);

}

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) {

}

/// Visit a node of ASTAssignExpression. 
/// 
/// @param node     a node of ASTAssignExpression
void CminusfBuilder::visit(ASTAssignExpression &node) {

    node.expression->accept(*this);
    auto value = cur_val;

    node.var->accept(*this);

}

/// Visit a node of ASTSimpleExpression.
/// We need to do type transform if possible. 
/// @param node     a node of ASTSimpleExpression
void CminusfBuilder::visit(ASTSimpleExpression &node) {

    // check whether the additive expressions are linked by relational operator
    // 1. just an additive expression
    if (!node.additive_expression_r) {
        node.additive_expression_l->accept(*this);
    }
    // 2. two additive expressions
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
                assert( nullptr && "strange relational operator type");
                break;
        }
        // extend???
        cur_val = result;
    }
}

/// Visit a node of ASTAdditiveExpression.
/// We need to do type transform if possible. 
/// @param node     a node of ASTAdditiveExpression
void CminusfBuilder::visit(ASTAdditiveExpression &node) {

    // the additive expression is only made up of term
    if (!node.additive_expression) {
        node.term->accept(*this);
    }
    // the additive expression is linked by additive operator
    // similar to the case in visiting ASTSimpleExpression
    else {
        node.additive_expression->accept(*this);
        auto l_val = cur_val;
        node.term->accept(*this);
        auto r_val = cur_val;
        bool is_int = Is_int_operation(builder, &l_val, &r_val);

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

    // the term is only made up of factor
    if (!node.term) {
        node.factor->accept(*this);
    }
    // the term is linked by a multiply operator
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
        cur_val = result;
    }
}

/// Visit a node of ASTCall
/// We need to do type transform if possible. 
/// @param node     a node of ASTCall
void CminusfBuilder::visit(ASTCall &node) {

}


Type* CminusType_to_IRType(CminusType cminus_type) {

    Type* IR_type;
    switch (cminus_type) {
        case TYPE_INT:
            IR_type = INT_32_TYPE;
            break;
        case TYPE_FLOAT:
            IR_type = FLOAT_32_TYPE;
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
            builder->create_sitofp(*l_val, FLOAT_32_TYPE);
        }
        if (r_type->is_integer_type()) {
            builder->create_sitofp(*r_val, FLOAT_32_TYPE);
        }
    }

    return is_int;
}