#pragma once
#include "enums.hpp"
#include "MainStack.hpp"
#include "HashMap.hpp"

class CodeGenerator {
public:
    CodeGenerator();

    void gen_arithmetic(char op);

    void gen_assignment();

    void gen_declare(VarType type, bool assignment);

    void gen_for_begin();

    void gen_for_end();

    void set_cond_expr_op(CondExprOp op);

public:
    MainStack stack;

private:
    HashMap<std::string, SymbolInfo> symbolTable;
    std::stack<std::string> label_stack;
    std::stringstream data_region;
    std::stringstream text_region;

    int label_counter = 0;
    CondExprOp cond_expr_op = CondExprOp::EQ;
};
