#pragma once
#include "enums.hpp"
#include "MainStack.hpp"
#include "HashMap.hpp"

class Compiler {
public:
    Compiler();

    void gen_arithmetic(char op);

    void gen_assignment();

    void gen_declare(VarType type, bool assignment);

    void gen_if_begin();

    void gen_if_end();

    void gen_else();

    void gen_for_begin();

    void gen_for_end();

    void gen_print(VarType print_type);

    void set_cond_expr_op(CondExprOp op);

    void set_for_conditions(std::string idx_id, bool inclusive, int increment = 1);

    void write_data_region(std::ostream &ostream) const;

    void write_text_region(std::ostream &ostream) const;

public:
    MainStack stack;

private:
    std::string gen_load_to_register(StackEntry &entry, std::optional<std::string_view> reg = std::nullopt);

    void gen_load_to_register(int value, std::string_view reg);

    void gen_cvt_i32_to_f32(std::string_view i_reg, std::string_view f_reg);

    void gen_cvt_f32_to_i32(std::string_view f_reg, std::string_view i_reg);

    void gen_store_to_variable(const StackEntry &var, std::string_view reg);

    std::string reserve_label();

private:
    std::stringstream data_region;
    std::stringstream text_region;
    HashMap<std::string, SymbolInfo> symbolTable;

    std::stack<std::string> label_stack;

    int label_counter = 0;
    CondExprOp cond_expr_op = CondExprOp::EQ;
    bool for_inclusive = false;
    int for_increment = 1;
};

