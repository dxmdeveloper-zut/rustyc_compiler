#include "Compiler.hpp"

Compiler::Compiler(): stack(symbolTable) {
}

void Compiler::gen_arithmetic(char op) {
    static int counter = 0;

    std::string result_name = "result_" + std::to_string(counter);
    std::string result_symbol = "__tmp_" + result_name;

    auto [lhs, rhs] = stack.pop_two();

    bool to_float = lhs.var_type == VarType::F32;
    to_float |= rhs.var_type == VarType::F32;

    auto result_var_type = to_float ? VarType::F32 : VarType::I32;
    auto instr_postfix = result_var_type == VarType::F32 ? ".s" : "";

    // Add result name tmp variable to symbol table
    symbolTable[result_symbol] = {result_var_type, true};

    // Load left and right operands into registers
    auto rhs_reg = gen_load_to_register(rhs, "$t0"); // TODO: automatic register selection
    auto lhs_reg = gen_load_to_register(lhs, "$t1");

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
    if (!found_sym.value()->initialized) {
        if (rhs.type == ExprElemType::NUMBER) {
            symbolTable[lhs.value].initial_value = rhs.value;
        } else if (rhs.var_type == VarType::U8_ARR) {
            std::string string = symbolTable[rhs.value].initial_value;
            symbolTable[lhs.value].initial_value = string;

            if (rhs.value.rfind("__str", 0) == 0) {
                symbolTable.erase(rhs.value);
            }
        }
        symbolTable[lhs.value].initialized = true;
    }
    else {
        decltype(gen_load_to_register(rhs)) rhs_reg;
        gen_store_to_variable(lhs, rhs_reg);
    }
}

void Compiler::gen_declare(VarType type, bool assignment) {
    StackEntry lhs = stack.top();
    std::string &symbol = lhs.value;

    if (symbolTable.contains(symbol)) {
        throw std::runtime_error("variable redeclaration: " + symbol);
    }

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

void Compiler::set_cond_expr_op(CondExprOp op) {
    cond_expr_op = op;
}

std::string Compiler::gen_load_to_register(StackEntry &entry, std::optional<std::string_view> reg) {
    // TODO: register type assertion
    std::string_view reg_name = reg.value_or("$t0"); // TODO: get first free register. Create function for that

    text_region << "l" << entry.get_instr_postfix() << " "
    << reg_name << ", " << entry.value << std::endl;

    return std::string(reg_name);
}

void Compiler::gen_load_to_register(int value, std::string_view reg) {
    text_region << "li " << reg << ", " << value << std::endl;
}

void Compiler::gen_store_to_variable(const StackEntry &var, std::string_view reg) {
    text_region << "s" << var.get_instr_postfix()
    << " " << reg
    <<  " , "  << var.value << std::endl;
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
    for (auto &symbol : symbolTable) {
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
