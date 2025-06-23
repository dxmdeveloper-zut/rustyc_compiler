#pragma once
#include "MainStack.hpp"
#include "HashMap.hpp"
#include "RegisterManager.hpp"
#include "common.hpp"

class Compiler {
public:
    Compiler();

    void gen_arithmetic(char op);

    void gen_assignment();

    /// @param declare_id_name for declaration without initialization
    void gen_declare(VarType type, const std::string &declare_id_name="");

    void gen_if_begin();

    void gen_if_end();

    void gen_else();

    void gen_for_begin();

    void gen_for_end();

    void gen_print(VarType print_type);

    void add_idx_to_arr_idx_stack();

    void set_cond_expr_op(CondExprOp op);

    void set_for_conditions(const std::string &idx_id, bool inclusive, int increment = 1);

    void write_data_region(std::ostream &ostream) const;

    void write_text_region(std::ostream &ostream) const;

    void gen_calc_arr_addr(bool extract);

public:
    MainStack stack;
    RegisterManager reg_mgr;
    HashMap<std::string, SymbolInfo> symbolTable;
    std::stack<int32_t> static_array_dims;

private:
    void declare_array(VarType type, const std::string &id);

    Reg gen_load_to_register(const StackEntry &entry, const char *reg_name = nullptr);

    [[nodiscard]] Reg gen_load_to_register(const StackEntry &entry, bool calc_result);

    [[nodiscard]] Reg gen_load_addr_to_register(const std::string &id);

    void gen_load_to_register(int value, std::string_view reg);

    void gen_cvt_i32_to_f32(const Reg &i_reg, const Reg &f_reg);

    void gen_cvt_f32_to_i32(const Reg &f_reg, const Reg &i_reg);

    [[nodiscard]] Reg gen_cvt_i32_to_f32(const Reg &i_reg);

    [[nodiscard]] Reg gen_cvt_f32_to_i32(const Reg &f_reg);

    void gen_auto_reg_type_unify(Reg &reg1, Reg &reg2);

    void gen_store_to_variable(const StackEntry &var, Reg &&reg);

    std::string reserve_label();

    static int32_t static_calculation(char op, const StackEntry &lhs, const StackEntry &rhs);

private:
    using StoringType = RegisterManager::StoringType;

    std::stringstream data_region;
    std::stringstream text_region;

    std::stack<std::string> label_stack;
    std::stack<StackEntry> arr_idx_stack;

    int label_counter = 0;
    CondExprOp cond_expr_op = CondExprOp::EQ;
    bool for_inclusive = false;
    int for_increment = 1;
    int loop_depth = 0;
    int tmp_counter = 0;
};




