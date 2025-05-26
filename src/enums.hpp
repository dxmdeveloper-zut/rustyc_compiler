#pragma once
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <optional>
#include <array>
#include <vector>
#include <stack>
#include <unordered_map>


enum class ExprElemType {
    ID,
    NUMBER,
    STRING_LITERAL, // only as argument to push it on the stack
};

enum class VarType {
    UNDEFINED,
    I32,
    F32,
    U8_ARR
};

enum class CondExprOp {
    EQ,
    NEQ,
    LT,
    LEQ,
    GT,
    GEQ
};

struct StackEntry {
    std::string value;
    ExprElemType type;
    VarType var_type = VarType::UNDEFINED;
};

struct SymbolInfo {
    VarType type;
    bool temporary;
    std::string inital_value;
};



void dbg_print_stack(std::stack<StackEntry> stack) {
    std::cerr << std::endl << "Stack size: " << stack.size() << std::endl;

    std::vector<StackEntry> tmp_stack(stack.size());
    std::size_t i = 0;
    while (stack.size() > 0) {
        tmp_stack[i++] = stack.top();
        stack.pop();
    }
    for (const auto &entry : tmp_stack) {
        std::cerr << entry.value << std::endl;
        stack.push(entry);
    }
    std::cerr << std::endl;
}

void push_string_literal(std::string value) {
    static int str_counter = 0;
    std::string symbol_name = "__str" + std::to_string(str_counter);
    symbolTable[symbol_name] = {VarType::U8_ARR, false, value};
    stack.push({symbol_name, ExprElemType::ID, VarType::U8_ARR});
    str_counter++;
}

void push_id(std::string value) {
    auto type = (symbolTable.find(value) == symbolTable.end()
                   ? VarType::UNDEFINED : symbolTable[value].type);
    stack.push({value, ExprElemType::ID, type});
}

std::string reserve_label() {
    std::string label_name = "L";
    label_name += std::to_string(label_counter++);
    label_stack.push(label_name);
    return label_name;
}

std::string load_to_register(StackEntry &entry, std::optional<std::string> reg = std::nullopt) {
    std::string reg_name = reg.value_or("$t0"); // TODO: get first free register. Create function for that

    // TODO: float support
    if (entry.type == ExprElemType::NUMBER) {
        text_region << "li " << reg_name << ", " << entry.value << std::endl;
    } else if (entry.type == ExprElemType::ID) {
        text_region << "lw " << reg_name << ", " << entry.value << std::endl;
    }
    return reg_name;

}

void gen_code_if_begin(CondExprOp condExprOp) {
    // TODO type mismatch check
    std::string jump_label = reserve_label();

    std::array<std::string_view, 6> branch_instr = {
        "bne", // CondExprOp::EQ
        "beq", // CondExprOp::NEQ
        "bge", // CondExprOp::LT
        "bgt", // CondExprOp::LEQ
        "ble", // CondExprOp::GT
        "blt"  // CondExprOp::GEQ
    };

    auto rhs = stack.top();
    stack.pop();
    auto lhs = stack.top();
    stack.pop();

    std::string lhs_reg = load_to_register(lhs);
    std::string rhs_reg = load_to_register(rhs, "$t1"); // TODO: get first free register. Modify load_to_register function to do that

    text_region << branch_instr[int(condExprOp)]
                << " " << lhs_reg << ", " << rhs_reg << ", " << jump_label << std::endl;
}

void gen_code_if_end() {
    std::string label = label_stack.top();
    label_stack.pop();
    text_region << label << ":" << std::endl;
}

void gen_code_print(VarType print_type) {
    auto stack_elem = stack.top();
    stack.pop();

    if (stack_elem.type == ExprElemType::ID) {
        if (symbolTable.find(stack_elem.value) == symbolTable.end()) {
            throw std::runtime_error("Undefined variable");
        }
    }

    switch (print_type) {
        case VarType::I32:
            text_region << "li $v0, 1" << std::endl
             << "l" << (stack_elem.type == ExprElemType::ID ? "w" : "i") // TODO: change it
             <<" $a0, " << stack_elem.value << std::endl;
            break;
        case VarType::U8_ARR:
            text_region << "li $v0, 4" << std::endl
              << "la $a0, " << stack_elem.value << std::endl;
            break;
        default:
            throw std::runtime_error("gen_code_print unsupported type");
    }
    text_region << "syscall" << std::endl;
}

void write_data_region(std::stringstream &data_region) {
    data_region << ".data:" << std::endl;
    for (auto &symbol : symbolTable) {
        data_region << symbol.first << ":    ";
        auto value = symbol.second.inital_value;

        switch (symbol.second.type) {
            case VarType::I32:
                data_region << ".word    ";
                break;
            case VarType::F32:
                data_region << ".float    ";
                break;
            case VarType::U8_ARR:
                data_region << ".asciiz    ";
                break;
            default:
                throw std::runtime_error("unsupported type");
        }

        data_region << (value.length() == 0 ? "0" : value) << std::endl;
    }
}

// functions
void gen_code_declare(VarType type, bool assignment) {

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
    symbolTable[symbol] = { type, false, entry.value };
}

void gen_code(char op) {
    static int counter = 0;

    std::string result_name = "result_" + std::to_string(counter);
    std::string result_symbol = "__tmp_" + result_name;

    auto lhs = stack.top();
    threes << result_name << " = " << lhs.value;
    stack.pop();
    auto rhs = stack.top();
    threes << " " << op << " " << rhs.value << std::endl;
    stack.pop();

    if((rhs.var_type == VarType::U8_ARR || lhs.var_type == VarType::U8_ARR)
        && (op != '=' || rhs.var_type != lhs.var_type)
        && (lhs.var_type != VarType::UNDEFINED) ) {
        throw std::runtime_error("arithmetic operation on string is not allowed");
    }

    bool to_float = lhs.var_type == VarType::F32;
    to_float |= rhs.var_type == VarType::F32;

    auto result_var_type = to_float ? VarType::F32 : VarType::I32;

    stack.push({result_symbol, ExprElemType::ID, result_var_type});

    // Add result name tmp variable to symbol table
    symbolTable[result_symbol] = {result_var_type, true};

    // Generate MIPS assembly code
    auto load_to_register = [](ExprElemType type, const std::string &reg, const std::string &value) -> std::string {
        if (type == ExprElemType::NUMBER)
            return "li " + reg + ", " + value + "\n";
        if (type == ExprElemType::ID)
            return "lw " + reg + ", " + value + "\n";
        return "";
    };
    auto simple_arithmetic = [&](//const StackEntry &lhs,
                                 //const StackEntry &rhs,
                                 const std::string &arithm
                                 ) -> std::string {
        std::stringstream ss;
        // load lhs to t0
        ss << load_to_register(rhs.type, "$t0", rhs.value);
        // load rhs to t1
        ss << load_to_register(lhs.type, "$t1", lhs.value);
        // perform arithmetic operation
        ss << arithm << " $t0, $t0, $t1" << std::endl;
        return ss.str();
    };


    switch(op) {
        case '-':
            text_region << simple_arithmetic("sub");
            break;
        case '+':
            text_region << simple_arithmetic("add");
            break;
        case '*':
            text_region << simple_arithmetic("mul");
            break;
        case '/':
            text_region << simple_arithmetic("div");
            break;
        case '=':
            // TODO: test i32 a = b;
            if (symbolTable.find(lhs.value) == symbolTable.end()) {
                // add symbol to the symbolTable as temporary
                symbolTable[lhs.value] = {rhs.var_type, true};
                if (rhs.type == ExprElemType::NUMBER) {
                    symbolTable[lhs.value].inital_value = rhs.value;
                    break;
                } else if (rhs.var_type == VarType::U8_ARR) {
                    std::string string = symbolTable[rhs.value].inital_value;
                    symbolTable[lhs.value].inital_value = string;

                    if (rhs.value.rfind("__str", 0) == 0) {
                        symbolTable.erase(rhs.value);
                    }
                    break;
                } else if (rhs.type == ExprElemType::ID) {
                    text_region << "lw $t0, " << rhs.value << "\n";
                }
            }
            text_region << "sw $t0, " << lhs.value << std::endl;
            break;
    }

    // TODO Optimize: used registers table, registers on stack
    if (op != '=') {
        text_region << "sw $t0, " << result_symbol << "\n";
        counter++;
    }

}

int main(int argc, char **argv) {
    yyparse();

    write_data_region(data_region);
    std::cout << data_region.str() << std::endl;
    std::cout << ".text:" << std::endl;
    std::cout << text_region.str();
    return 0;
}