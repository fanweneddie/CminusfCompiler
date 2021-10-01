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

/// Transfrom a Cminus type to IR type. 
/// Cminus type can only be int, float or void. 
/// Otherwise, the returned type will be nullptr 
/// @param cminus_type      a Cminus type 
/// @return                 the corresponding IR type 
Type* CminusType_to_IRType(CminusType cminus_type);

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

void CminusfBuilder::visit(ASTNum &node) {

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
/// Here we just need to continue exploring the ASTExpreesion node
/// @param node     a node of ASTExpressionStmt
void CminusfBuilder::visit(ASTExpressionStmt &node) {
    
    auto expr = node.expression;
    if (expr) {
        expr->accept(*this);
    }
}

/// Visit a node of ASTSelectionStmt. 
/// Here we just need to continue visiting 
/// @param node     a node of ASTExpressionStmt
void CminusfBuilder::visit(ASTSelectionStmt &node) {
}

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) { }

void CminusfBuilder::visit(ASTAssignExpression &node) { }

void CminusfBuilder::visit(ASTSimpleExpression &node) { }

void CminusfBuilder::visit(ASTAdditiveExpression &node) { }

void CminusfBuilder::visit(ASTTerm &node) { }

void CminusfBuilder::visit(ASTCall &node) { }


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