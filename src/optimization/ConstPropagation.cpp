#include "ConstPropagation.hpp"
#include "Value.h"
#include "Instruction.h"
#include <map>

// we don't deal with global variables or pointer elements
#define IS_GLOBAL_VARIABLE(l_val) dynamic_cast<GlobalVariable *>(l_val) != nullptr
#define IS_GEP_INSTR(l_val) dynamic_cast<GetElementPtrInst *>(l_val) != nullptr
// check the type of each instruction
#define IS_BINARY_INSTR(instr) dynamic_cast<BinaryInst *>(instr) != nullptr
#define IS_CMP_INSTR(instr) dynamic_cast<CmpInst *>(instr) != nullptr
#define IS_FCMP_INSTR(instr) dynamic_cast<FCmpInst *>(instr) != nullptr

/*
 * Process a constant propagation on a binary instruction
 * @param instr: the binary instruction to be processed
 * @param program_state: the global map that stores the state of each variable
 * @return: whether the state has changed
 */
bool process_bin_instr(BinaryInst *instr,
                       std::map<Instruction, std::map<Value*, ConstState>>& program_state) {
    //auto r_val_1 = instr->get_operand(0);
    //auto r_val_2 = instr->get_operand(1);
    if (instr->is_int_instr()) {

    } else if (instr->is_fp_instr()) {

    }
}

/*
 * Process a constant propagation on an integer comparison instruction
 * @param instr: the integer comparison instruction to be processed
 * @param program_state: the global map that stores the state of each variable
 * @return: whether the state has changed
 */
bool process_cmp_instr(CmpInst *instr,
                       std::map<Instruction, std::map<Value*, ConstState>>& program_state) {
    return false;
}

/*
 * Process a constant propagation on a float comparison instruction
 * @param instr: the float comparison instruction to be processed
 * @param program_state: the global map that stores the state of each variable
 * @return: whether the state has changed
 */
bool process_fcmp_instr(FCmpInst *instr,
                       std::map<Instruction, std::map<Value*, ConstState>>& program_state) {
    return false;
}

/*
 * Process a constant propagation on an instruction, based on its type
 * @param instr: the instruction to be processed
 * @param program_state: the global map that stores the state of each variable
 * @return: whether the state has changed
 */
bool process_instr(Instruction *instr,
                   std::map<Instruction, std::map<Value*, ConstState>>& program_state) {
    // we only analyze binary and comparison instruction
    if (IS_BINARY_INSTR(instr)) {
        auto bin_instr = dynamic_cast<BinaryInst *>(instr);
        return process_bin_instr(bin_instr, program_state);
    } else if (IS_CMP_INSTR(instr)) {
        auto cmp_instr = dynamic_cast<CmpInst *>(instr);
        return process_cmp_instr(cmp_instr, program_state);
    } else if (IS_FCMP_INSTR(instr)) {
        auto fcmp_instr = dynamic_cast<FCmpInst *>(instr);
        return process_fcmp_instr(fcmp_instr, program_state);
    } else {
        return false;
    }
}

void ConstPropagation::process_func(Function *func) {
    // we don't deal with an empty function
    if (func->get_num_basic_blocks() == 0) {
        return;
    }

    // maps the constant state of each value at each program point
    std::map<Instruction, std::map<Value*, ConstState>> program_state;
    // the worklist to store basic blocks
    std::list<BasicBlock*> bb_list;

    // the worklist algorithm
    auto bb = func->get_entry_block();
    bb_list.push_back(bb);
    while (bb_list.size() > 0) {
        bb = bb_list.front();
        bb_list.pop_front();
        for (auto instr : bb->get_instructions()) {
            if (process_instr(instr, program_state)) {

            }
        }
    }
}

void ConstPropagation::run() {
    // do each intraprocedural analysis separately
    for (auto func : m_->get_functions()) {
        process_func(func);
    }
}