#include "Compiler.hpp"

#include <cassert>

Compiler::Compiler(): stack(symbolTable), reg_mgr(this) {
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
    Reg rhs_reg = gen_load_to_register(rhs, true);
    Reg lhs_reg = gen_load_to_register(lhs);

    if (to_float) {
        if (rhs.var_type == VarType::I32) {
            rhs_reg = gen_cvt_i32_to_f32(rhs_reg);
        } else {
            lhs_reg = gen_cvt_i32_to_f32(lhs_reg);
        }
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

    // push result to stack
    stack.push({result_symbol, ExprElemType::ID, result_var_type});

    gen_store_to_variable(stack.top(), std::move(rhs_reg));
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

    // assignment to uninitialized variable
    bool assigned_statically = false;
    if (!found_sym.value()->initialized) {
        SymbolInfo* r_sym = nullptr;
        if (rhs.type != ExprElemType::NUMBER)
            r_sym = &symbolTable.at(rhs.value);

        if (rhs.type == ExprElemType::NUMBER) {
            found_sym.value()->initial_value = rhs.value;
            assigned_statically = true;
        } else if (rhs.value.rfind("__str") == 0) {
            found_sym.value()->initial_value = r_sym->initial_value;
            symbolTable.erase(rhs.value);
            assigned_statically = true;
        } else if (rhs.value.rfind("__fl") == 0) {
            if (lhs.var_type == VarType::I32)
                r_sym->initial_value = std::to_string(std::stoi(r_sym->initial_value));
            found_sym.value()->initial_value = r_sym->initial_value;
            symbolTable.erase(rhs.value);
            assigned_statically = true;
        }
        found_sym.value()->initialized = true;
    } else if (lhs.var_type == VarType::U8_ARR) {
        throw std::runtime_error("reassignment of string is not allowed");
    }

    if (!assigned_statically) {
        auto rhs_reg = gen_load_to_register(rhs);
        // Conversion of right side to match left side variable type
        if (lhs.var_type == VarType::I32 && rhs.var_type == VarType::F32) {
            rhs_reg = gen_cvt_f32_to_i32(rhs_reg);
        } else if (lhs.var_type == VarType::F32 && rhs.var_type == VarType::I32) {
            rhs_reg = gen_cvt_i32_to_f32(rhs_reg);
        }
        gen_store_to_variable(lhs, std::move(rhs_reg));
    }

    //
    // if (!found_sym.value()->initialized && rhs.type != ExprElemType::ID) {
    //     if (rhs.type == ExprElemType::NUMBER) {
    //         if (lhs.var_type == VarType::I32 && rhs.var_type == VarType::F32) {
    //             // Convert float to int statically
    //             rhs.value = std::to_string(std::stoi(rhs.value));
    //         }
    //         symbolTable[lhs.value].initial_value = rhs.value;
    //     }
    //     symbolTable[lhs.value].initialized = true;
    // } else if (rhs.var_type == VarType::U8_ARR) {
    //     std::string string = symbolTable[rhs.value].initial_value;
    //     symbolTable[lhs.value].initial_value = string;
    //
    //     if (rhs.value.rfind("__str", 0) == 0) {
    //         symbolTable.erase(rhs.value);
    //     }
    // } else if (rhs.var_type == VarType::F32 && rhs.value.rfind("__") == 0) {
    //     std::string val = symbolTable[rhs.value].initial_value;
    //     symbolTable[lhs.value].initial_value = val;
    //
    //     symbolTable.erase(rhs.value);
    // } else {
    //     auto rhs_reg = gen_load_to_register(rhs);
    //     // Conversion of right side to match left side variable type
    //     if (lhs.var_type == VarType::I32 && rhs.var_type == VarType::F32) {
    //         rhs_reg = gen_cvt_f32_to_i32(rhs_reg);
    //     } else if (lhs.var_type == VarType::F32 && rhs.var_type == VarType::I32) {
    //         rhs_reg = gen_cvt_i32_to_f32(rhs_reg);
    //     }
    //     gen_store_to_variable(lhs, std::move(rhs_reg));
    // }
    reg_mgr.release_calc_results();
}

void Compiler::gen_declare(VarType type, std::string declare_id_name) {
    bool assignment = declare_id_name.empty();

    if (!assignment) {
        stack.push_id(declare_id_name, true);
    }

    std::string symbol(stack.top().value);


    if (symbolTable.contains(symbol)) {
        throw std::runtime_error("variable redeclaration: " + symbol);
    }

    stack.top().var_type = type;
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

    Reg lhs_reg = gen_load_to_register(lhs);
    Reg rhs_reg = gen_load_to_register(rhs);

    text_region << branch_instr[int(cond_expr_op)]
            << " " << lhs_reg.str() << ", " << rhs_reg.str() << ", " << jump_label << std::endl;
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

    reg_mgr.gen_dump_calc_results_to_memory(text_region);

    // Get index variable and right-hand side value from the stack
    auto [range_stop, idx] = stack.pop_two();
    Reg idx_reg = gen_load_to_register(idx);

    // write start of the loop label
    text_region << "b " << loop_body_label << std::endl;
    text_region << loop_start_label << ":" << std::endl;

    std::string idx_reg_str = idx_reg.str();
    idx_reg = gen_load_to_register(idx, idx_reg_str.c_str());

    // increment
    if (for_increment > 0) {
        text_region << "addi " << idx_reg << ", " << idx_reg << ", " << for_increment << std::endl;
    } else {
        text_region << "subi " << idx_reg << ", " << idx_reg << ", " << -for_increment << std::endl;
    }
    gen_store_to_variable(idx, std::move(idx_reg));

    // condition check
    Reg rhs_reg = gen_load_to_register(range_stop);

    std::string branch_instr;
    if (for_increment > 0)
        branch_instr = (for_inclusive ? "bgt" : "bge");
    else
        branch_instr = (for_inclusive ? "blt" : "ble");

    text_region << branch_instr << " " << idx_reg << ", " << rhs_reg << ", " << loop_end_label << std::endl;
    text_region << loop_body_label << ":" << std::endl;
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
    gen_declare(VarType::I32, idx_id);

    stack.push(ExprElemType::ID, idx_id, VarType::I32);
    stack.push(range_right);

    for_increment = increment;
    for_inclusive = inclusive;
}

Reg Compiler::gen_load_to_register(const StackEntry &entry, const char *reg_name) {
    Reg reg{};

    if (entry.type == ExprElemType::ID) {
        auto symbol = symbolTable.find(entry.value);
        assert(symbol.has_value());
        if (symbol.value()->occupied_reg) {
            if (reg_name == nullptr || symbol.value()->occupied_reg.str() == reg_name) {
                return symbol.value()->occupied_reg;
            } else {
                text_region << reg_name << ":" << std::endl;
                text_region << "l" << entry.get_instr_postfix() << " "
                << reg_name << ", " << symbol.value()->occupied_reg << std::endl;
            }
        }
    }

    if (reg_name == nullptr) {
        if (entry.var_type == VarType::I32) {
            reg = reg_mgr.get_free_register(Reg::Type::T_REG);
        } else if (entry.var_type == VarType::F32) {
            reg = reg_mgr.get_free_register(Reg::Type::F_REG);
        }
        if (!reg) {
            throw std::runtime_error("Out of registers");
        }
    } else reg = Reg(reg_name);

    assert(!(entry.var_type != VarType::F32 && reg.get_type() == Reg::Type::F_REG));

    text_region << "l" << entry.get_instr_postfix() << " "
            << reg.str() << ", " << entry.value << std::endl;

    return std::move(reg);
}

Reg Compiler::gen_load_to_register(const StackEntry &entry, bool calc_result) {
    Reg reg{};

    if (entry.type == ExprElemType::ID) {
        auto symbol = symbolTable.find(entry.value);
        assert(symbol.has_value());
        if (symbol.value()->occupied_reg) {
            return symbol.value()->occupied_reg;
        }
    }

    if (calc_result == false)
        return std::move(gen_load_to_register(entry));

    if (entry.var_type == VarType::I32)
        reg = reg_mgr.get_free_register(Reg::Type::T_REG, StoringType::CALC_RESULT);
    else if (entry.var_type == VarType::F32)
        reg = reg_mgr.get_free_register(Reg::Type::F_REG, StoringType::CALC_RESULT);

    if (!reg) {
        throw std::runtime_error("Out of registers");
    }

    text_region << "l" << entry.get_instr_postfix() << " "
            << reg.str() << ", " << entry.value << std::endl;

    return std::move(reg);
}

void Compiler::gen_load_to_register(int value, std::string_view reg) {
    text_region << "li " << reg << ", " << value << std::endl;
}

void Compiler::gen_cvt_i32_to_f32(const Reg &i_reg, const Reg &f_reg) {
    text_region << "mtc1 " << i_reg << ", " << f_reg << std::endl;
    text_region << "cvt.s.w " << f_reg << ", " << f_reg << std::endl;
}

void Compiler::gen_cvt_f32_to_i32(const Reg &f_reg, const Reg &i_reg) {
    text_region << "cvt.w.s " << f_reg << ", " << f_reg << std::endl;
    text_region << "mfc1 " << i_reg << ", " << f_reg << std::endl;
}

Reg Compiler::gen_cvt_i32_to_f32(const Reg &i_reg) {
    assert(i_reg.get_type() == Reg::Type::T_REG);
    Reg f_reg = reg_mgr.get_free_register(Reg::Type::F_REG);
    if (!f_reg) {
        throw std::runtime_error("Out of registers");
    }
    gen_cvt_i32_to_f32(i_reg, f_reg);
    return f_reg;
}

Reg Compiler::gen_cvt_f32_to_i32(const Reg &f_reg) {
    assert(f_reg.get_type() == Reg::Type::F_REG);
    Reg i_reg = reg_mgr.get_free_register(Reg::Type::T_REG);
    if (!i_reg) {
        throw std::runtime_error("Out of registers");
    }
    gen_cvt_f32_to_i32(f_reg, i_reg);
    return i_reg;
}

void Compiler::gen_store_to_variable(const StackEntry &var, Reg &&reg) {
    assert(reg);
    if (var.value.rfind("__tmp", 0) == 0) {
        if (reg_mgr.try_preserve_value(reg, StoringType::CALC_RESULT, var.value) == 0) {
            symbolTable.at(var.value).occupied_reg = std::move(reg);
            return;
        }
        symbolTable.at(var.value).tmp_in_data_region = true;
    }
    text_region << "s" << var.get_instr_postfix()
            << " " << reg
            << ", " << var.value << std::endl;
}

std::string Compiler::reserve_label() {
    std::string label_name = "L";
    label_name += std::to_string(label_counter++);
    label_stack.push(label_name);
    return label_name;
}

void Compiler::gen_print(VarType print_type) {
    auto stack_elem = stack.pop();

    // Check type
    if (!((stack_elem.var_type == VarType::I32 && print_type == VarType::I32)
        || stack_elem.var_type == VarType::F32 && print_type == VarType::F32
        || stack_elem.var_type == VarType::U8_ARR && print_type == VarType::U8_ARR)) {
        throw std::runtime_error("Invalid print argument type");
    }

    switch (print_type) {
        case VarType::I32:
            gen_load_to_register(1, "$v0");
            gen_load_to_register(stack_elem, "$a0");
            break;
        case VarType::F32:
            gen_load_to_register(2, "$v0");
            gen_load_to_register(stack_elem, "$f12");
            break;
        case VarType::U8_ARR:
            gen_load_to_register(4, "$v0");
            gen_load_to_register(stack_elem, "$a0");
            break;
        default:
            throw std::runtime_error("gen_code_print unsupported type");
    }
    text_region << "syscall" << std::endl;
}

void Compiler::declare_array(VarType type) {
    assert(!static_array_dims.empty());
    auto id = stack.pop();

    if (symbolTable.contains(id.value))
        throw std::runtime_error("Symbol already exists");

    size_t total_size = 0;
    int32_t cur_size = 1;
    std::vector<int32_t> dims;
    std::vector<int32_t> sizes;
    dims.reserve(static_array_dims.size());
    sizes.reserve(static_array_dims.size());

    for (int i = 0; i < static_array_dims.size(); i++) {
        auto size = static_array_dims.top();
        if (size <= 0) {
            throw std::runtime_error("Array size must be positive");
        }
        dims.push_back(size);
        sizes.push_back(cur_size);
        cur_size *= size;
        total_size += size;
    }

    SymbolInfo symbol{
        .type = type,
        .temporary = false,
        .initial_value = "0:" + total_size,
        .array_dims = dims,
        .array_sizes = sizes,
        .initialized = true
    };
}

void Compiler::write_data_region(std::ostream &ostream) const {
    ostream << ".data:" << std::endl;
    for (auto &symbol: symbolTable) {
        assert(!(symbol.second.temporary && symbol.first[0] != '_'));
        if (symbol.second.temporary && !symbol.second.tmp_in_data_region)
            continue;

        ostream << symbol.first << ":    ";
        auto value = symbol.second.initial_value;

        switch (symbol.second.type) {
            case VarType::I32:
            case VarType::I32_ARR:
                ostream << ".word    ";
                break;
            case VarType::F32:
            case VarType::F32_ARR:
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
