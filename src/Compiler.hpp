#pragma once
#include "MainStack.hpp"
#include "HashMap.hpp"
#include "RegisterManager.hpp"
#include "enums.hpp"

class Compiler {
public:
    Compiler();

    void gen_arithmetic(char op);

    void gen_assignment();

    /// @param declare_id_name for declaration without initialization
    void gen_declare(VarType type, std::string declare_id_name="");

    void gen_if_begin();

    void gen_if_end();

    void gen_else();

    void gen_for_begin();

    void gen_for_end();

    void gen_print(VarType print_type);

    void declare_array(VarType type);

    void set_cond_expr_op(CondExprOp op);

    void set_for_conditions(std::string idx_id, bool inclusive, int increment = 1);

    void write_data_region(std::ostream &ostream) const;

    void write_text_region(std::ostream &ostream) const;

public:
    MainStack stack;
    RegisterManager reg_mgr;
    HashMap<std::string, SymbolInfo> symbolTable;
    std::stack<int32_t> static_array_dims;

private:
    Reg gen_load_to_register(const StackEntry &entry, const char *reg_name = nullptr);

    Reg gen_load_to_register(const StackEntry &entry, bool calc_result);

    void gen_load_to_register(int value, std::string_view reg);

    void gen_cvt_i32_to_f32(const Reg &i_reg, const Reg &f_reg);

    void gen_cvt_f32_to_i32(const Reg &f_reg, const Reg &i_reg);

    [[nodiscard]] Reg gen_cvt_i32_to_f32(const Reg &i_reg);

    [[nodiscard]] Reg gen_cvt_f32_to_i32(const Reg &f_reg);

    void gen_store_to_variable(const StackEntry &var, Reg &&reg);

    std::string reserve_label();

private:
    using StoringType = RegisterManager::StoringType;

    std::stringstream data_region;
    std::stringstream text_region;

    std::stack<std::string> label_stack;

    int label_counter = 0;
    CondExprOp cond_expr_op = CondExprOp::EQ;
    bool for_inclusive = false;
    int for_increment = 1;

};

