#include "Compiler.hpp"

#include <assert.h>

Compiler::Compiler(): stack(symbolTable) {
}

void Compiler::gen_arithmetic(char op) {
    static int counter = 0;

    std::string result_name = "result_" + std::to_string(counter);
    std::string result_symbol = "__tmp_" + result_name;

    auto [lhs, rhs] = stack.pop_two();

    bool to_float = lhs.var_type == VarType::F32;
    to_float |= rhs.var_type == VarType::F32;
    to_float &= rhs.var_type != lhs.var_type;

    auto result_var_type = to_float | rhs.var_type == VarType::F32 ? VarType::F32 : VarType::I32;
    auto instr_postfix = result_var_type == VarType::F32 ? ".s" : "";

    // Add result name tmp variable to symbol table
    symbolTable[result_symbol] = {result_var_type, true};

    // Load left and right operands into registers
    // TODO: automatic register selection
    std::string rhs_reg;
    std::string lhs_reg;

    if (to_float) {
        if (rhs.var_type == VarType::I32) {
            rhs_reg = gen_load_to_register(rhs, "$t0");
            gen_cvt_i32_to_f32(rhs_reg, "$f0");
            lhs_reg = gen_load_to_register(lhs, "$f1");
        } else {
            lhs_reg = gen_load_to_register(rhs, "$t1");
            gen_cvt_i32_to_f32(rhs_reg, "$f1");
            rhs_reg = gen_load_to_register(rhs, "$f0");
        }
    } else if (result_var_type == VarType::F32) {
        rhs_reg = gen_load_to_register(rhs, "$f0");
        lhs_reg = gen_load_to_register(lhs, "$f1");
    } else {
        rhs_reg = gen_load_to_register(rhs, "$t0");
        lhs_reg = gen_load_to_register(lhs, "$t1");
    }

    // write the operation to the text region
    switch (op) {
        case '-':
            text_region << "sub";
            break;
        case '+':
            text_region << "add";
            break;
        case '*':
            text_region << "mul";
            break;
        case '/':
            text_region << "div";
            break;
    }

    text_region << instr_postfix << " " << rhs_reg << ", " << rhs_reg << ", " << lhs_reg << std::endl;

    // TODO: TODO?
    stack.push({result_symbol, ExprElemType::ID, result_var_type});

    // TODO Optimize: used registers table, registers on stack
    gen_store_to_variable(stack.top(), rhs_reg);
    counter++;
}

void Compiler::gen_assignment() {
    auto [lhs, rhs] = stack.pop_two();

    if (lhs.type != ExprElemType::ID) {
        throw std::runtime_error("Left side of assignment must be an identifier");
    }

    auto found_sym = symbolTable.find(lhs.value);

    if (!found_sym.has_value()) {
        throw std::runtime_error("undefined variable: " + lhs.value);
    }
    if (!found_sym.value()->initialized && rhs.type != ExprElemType::ID) {
        if (rhs.type == ExprElemType::NUMBER) {
            if (lhs.var_type == VarType::I32 && rhs.var_type == VarType::F32) {
                // Convert float to int statically
                rhs.value = std::to_string(std::stoi(rhs.value));
            }
            symbolTable[lhs.value].initial_value = rhs.value;
        }
        symbolTable[lhs.value].initialized = true;
    } else if (rhs.var_type == VarType::U8_ARR) {
        std::string string = symbolTable[rhs.value].initial_value;
        symbolTable[lhs.value].initial_value = string;

        if (rhs.value.rfind("__str", 0) == 0) {
            symbolTable.erase(rhs.value);
        }
    } else if (rhs.var_type == VarType::F32 && rhs.value.rfind("__")) {
        std::string val = symbolTable[rhs.value].initial_value;
        symbolTable[lhs.value].initial_value = val;

        symbolTable.erase(rhs.value);
    } else {
        auto rhs_reg = gen_load_to_register(rhs);
        // Conversion of right side to match left side variable type
        if (lhs.var_type == VarType::I32 && rhs.var_type == VarType::F32) {
            gen_cvt_i32_to_f32(rhs_reg, "$f0");
            rhs_reg = "$f0";
        } else if (lhs.var_type == VarType::F32 && rhs.var_type == VarType::I32) {
            gen_cvt_f32_to_i32(rhs_reg, "$t0");
            rhs_reg = "$t0";
        }
        gen_store_to_variable(lhs, rhs_reg);
    }
}

void Compiler::gen_declare(VarType type, bool assignment) {
    StackEntry &lhs = stack.top();
    std::string &symbol = lhs.value;

    if (symbolTable.contains(symbol)) {
        throw std::runtime_error("variable redeclaration: " + symbol);
    }

    lhs.var_type = type;
    symbolTable[symbol] = {type, false};

    if (!assignment) {
        stack.pop();
        return;
    }
    gen_assignment();
}

void Compiler::gen_if_begin() {
    // TODO type mismatch check
    std::string jump_label = reserve_label();

    static_assert(static_cast<int>(CondExprOp::EQ) == 0
                  && static_cast<int>(CondExprOp::GEQ) == 5);
    std::array<std::string_view, 6> branch_instr = {
        "bne", // CondExprOp::EQ
        "beq", // CondExprOp::NEQ
        "bge", // CondExprOp::LT
        "bgt", // CondExprOp::LEQ
        "ble", // CondExprOp::GT
        "blt" // CondExprOp::GEQ
    };

    auto [lhs, rhs] = stack.pop_two();

    std::string lhs_reg = gen_load_to_register(lhs);
    std::string rhs_reg = gen_load_to_register(rhs, "$t1");
    // TODO: get first free register. Modify load_to_register function to do that

    text_region << branch_instr[int(cond_expr_op)]
            << " " << lhs_reg << ", " << rhs_reg << ", " << jump_label << std::endl;
}

void Compiler::gen_if_end() {
    std::string label = label_stack.top();
    label_stack.pop();
    text_region << label << ":" << std::endl;
}

void Compiler::gen_else() {
    std::string else_label = label_stack.top();
    label_stack.pop();
    std::string end_label = reserve_label();

    text_region << "b " << end_label << std::endl;
    text_region << else_label << ":" << std::endl;
}

void Compiler::gen_for_begin() {
    std::string loop_end_label = reserve_label();
    std::string loop_body_label = reserve_label();
    label_stack.pop();
    std::string loop_start_label = reserve_label();

    // Get index variable and right-hand side value from the stack
    auto [idx_id, rhs] = stack.pop_two();
    std::string idx_reg = gen_load_to_register(idx_id);

    // write start of the loop label
    text_region << loop_start_label << ":" << std::endl;

    idx_reg = gen_load_to_register(idx_id);

    // increment
    if (for_increment > 0) {
        text_region << "addi " << idx_reg << ", " << idx_reg << ", " << for_increment << std::endl;
    } else {
        text_region << "subi " << idx_reg << ", " << idx_reg << ", " << -for_increment << std::endl;
    }

    // condition check
    std::string rhs_reg = gen_load_to_register(rhs, "$t1");

    std::string branch_instr;
    if (for_increment > 0)
        branch_instr = (for_inclusive ? "bgt" : "bge");
    else
        branch_instr = (for_inclusive ? "blt" : "ble");

    text_region << branch_instr << " " << idx_reg << ", " << rhs_reg << ", " << loop_end_label << std::endl;
}

void Compiler::gen_for_end() {
    std::string loop_start_label = label_stack.top();
    label_stack.pop();
    std::string loop_end_label = label_stack.top();
    label_stack.pop();

    text_region << "b " << loop_start_label << std::endl;
    text_region << loop_end_label << ":" << std::endl;
}

void Compiler::set_cond_expr_op(CondExprOp op) {
    cond_expr_op = op;
}

void Compiler::set_for_conditions(std::string idx_id, bool inclusive, int increment) {
    auto range_right = stack.pop();
    // range left is in the stack top
    // declaration with assignment idx = range_left
    stack.push(ExprElemType::ID, idx_id, VarType::I32);
    gen_declare(VarType::I32, true);

    stack.push(ExprElemType::ID, idx_id, VarType::I32);
    stack.push(range_right);

    for_increment = increment;
    for_inclusive = inclusive;
}

std::string Compiler::gen_load_to_register(StackEntry &entry, std::optional<std::string_view> reg) {
    // TODO: register type assertion
    std::string_view reg_name = reg.value_or(entry.var_type == VarType::F32 ? "$f0" : "$t0");
    // TODO: get first free register. Create function for that

    text_region << "l" << entry.get_instr_postfix() << " "
            << reg_name << ", " << entry.value << std::endl;

    return std::string(reg_name);
}

void Compiler::gen_load_to_register(int value, std::string_view reg) {
    text_region << "li " << reg << ", " << value << std::endl;
}

void Compiler::gen_cvt_i32_to_f32(std::string_view i_reg, std::string_view f_reg) {
    text_region << "mtc1 " << i_reg << ", " << f_reg << std::endl;
    text_region << "cvt.s.w " << f_reg << ", " << f_reg << std::endl;
}

void Compiler::gen_cvt_f32_to_i32(std::string_view f_reg, std::string_view i_reg) {
    text_region << "cvt.w.s " << f_reg << ", " << f_reg << std::endl;
    text_region << "mfc1 " << i_reg << ", " << f_reg << std::endl;
}

void Compiler::gen_store_to_variable(const StackEntry &var, std::string_view reg) {
    assert(!reg.empty());
    text_region << "s" << var.get_instr_postfix()
            << " " << reg
            << " , " << var.value << std::endl;
}

std::string Compiler::reserve_label() {
    std::string label_name = "L";
    label_name += std::to_string(label_counter++);
    label_stack.push(label_name);
    return label_name;
}

void Compiler::gen_print(VarType print_type) {
    auto stack_elem = stack.pop();

    if (stack_elem.type == ExprElemType::ID && !symbolTable.contains(stack_elem.value)) {
        throw std::runtime_error("Undefined variable");
    }

    switch (print_type) {
        case VarType::I32:
            gen_load_to_register(1, "$v0");
            gen_load_to_register(stack_elem, "$a0");
            break;
        case VarType::F32:
            gen_load_to_register(2, "$v0");
            gen_load_to_register(stack_elem, "$f12");
        case VarType::U8_ARR:
            gen_load_to_register(4, "$v0");
            gen_load_to_register(stack_elem, "$a0");
            break;
        default:
            throw std::runtime_error("gen_code_print unsupported type");
    }
    text_region << "syscall" << std::endl;
}

void Compiler::write_data_region(std::ostream &ostream) const {
    ostream << ".data:" << std::endl;
    for (auto &symbol: symbolTable) {
        ostream << symbol.first << ":    ";
        auto value = symbol.second.initial_value;

        switch (symbol.second.type) {
            case VarType::I32:
                ostream << ".word    ";
                break;
            case VarType::F32:
                ostream << ".float    ";
                break;
            case VarType::U8_ARR:
                ostream << ".asciiz    ";
                break;
            default:
                throw std::runtime_error("unsupported type");
        }

        ostream << (value.empty() ? "0" : value) << std::endl;
    }
    ostream << std::endl;
}

void Compiler::write_text_region(std::ostream &ostream) const {
    ostream << ".text:" << std::endl;
    ostream << text_region.str() << std::endl;
}
