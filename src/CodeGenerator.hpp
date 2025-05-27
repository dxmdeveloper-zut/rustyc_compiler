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

    void gen_if_begin();

    void gen_if_end();

    void gen_print(VarType print_type);

    void set_cond_expr_op(CondExprOp op);

public:
    MainStack stack;

private:
    std::string gen_load_to_register(StackEntry &entry, std::optional<std::string_view> reg = std::nullopt);

    void gen_load_to_register(int value, std::string_view reg);

    void gen_store_to_variable(const StackEntry &var, std::string_view reg);

    std::string reserve_label();

private:
    HashMap<std::string, SymbolInfo> symbolTable;
    std::stack<std::string> label_stack;
    std::stringstream data_region;
    std::stringstream text_region;

    int label_counter = 0;
    CondExprOp cond_expr_op = CondExprOp::EQ;
};
