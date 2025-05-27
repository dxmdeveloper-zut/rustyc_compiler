#include "CodeGenerator.hpp"

CodeGenerator::CodeGenerator(): stack(symbolTable) {
}

void CodeGenerator::gen_arithmetic(char op) {
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

void CodeGenerator::gen_assignment() {
    auto [lhs, rhs] = stack.pop_two();

    if (lhs.type != ExprElemType::ID) {
        throw std::runtime_error("Left side of assignment must be an identifier");
    }

    auto found_sym = symbolTable.find(lhs.value);

    decltype(gen_load_to_register(rhs)) rhs_reg;

    if (!found_sym.has_value()) {
        // add symbol to the symbolTable as temporary
        symbolTable[lhs.value] = {rhs.var_type, true};
        if (rhs.type == ExprElemType::NUMBER) {
            symbolTable[lhs.value].initial_value = rhs.value;
        } else if (rhs.var_type == VarType::U8_ARR) {
            std::string string = symbolTable[rhs.value].initial_value;
            symbolTable[lhs.value].initial_value = string;

            if (rhs.value.rfind("__str", 0) == 0) {
                symbolTable.erase(rhs.value);
            }
        } else if (rhs.type == ExprElemType::ID) {
            rhs_reg = gen_load_to_register(rhs);
        }
    }

    gen_store_to_variable(lhs, rhs_reg);
}

void CodeGenerator::gen_declare(VarType type, bool assignment) {
    StackEntry entry = stack.top();
    stack.pop();
    std::string &symbol = entry.value;

    if (assignment) {
        if (!symbolTable[symbol].temporary)
            throw std::runtime_error("variable redeclaration");

        symbolTable[symbol].temporary = false;

        if (symbolTable[symbol].type == type) {
            // TODO: convert if needed
            throw std::runtime_error("conversion is not implemented");
        }
        return;
    }
    symbolTable[symbol] = {type, false, entry.value};
}

void CodeGenerator::gen_if_begin() {

}

void CodeGenerator::set_cond_expr_op(CondExprOp op) {
    cond_expr_op = op;
}

std::string CodeGenerator::gen_load_to_register(StackEntry &entry, std::optional<std::string_view> reg) {
    // TODO: register type assertion
    std::string_view reg_name = reg.value_or("$t0"); // TODO: get first free register. Create function for that

    text_region << "l" << entry.get_instr_postfix() << " "
    << reg_name << ", " << entry.value << std::endl;

    return std::string(reg_name);
}

void CodeGenerator::gen_load_to_register(int value, std::string_view reg) {
    text_region << "li " << reg << ", " << value << std::endl;
}

void CodeGenerator::gen_store_to_variable(const StackEntry &var, std::string_view reg) {
    text_region << "s" << var.get_instr_postfix()
    << " " << reg
    <<  " , "  << var.value << std::endl;
}

std::string CodeGenerator::reserve_label() {
    std::string label_name = "L";
    label_name += std::to_string(label_counter++);
    label_stack.push(label_name);
    return label_name;
}

void CodeGenerator::gen_print(VarType print_type) {
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

