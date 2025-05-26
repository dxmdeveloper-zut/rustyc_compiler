#include "CodeGenerator.hpp"

CodeGenerator::CodeGenerator():
    stack(symbolTable) {}

void CodeGenerator::gen_arithmetic(char op) {
    static int counter = 0;

    std::string result_name = "result_" + std::to_string(counter);
    std::string result_symbol = "__tmp_" + result_name;

    auto [lhs, rhs] = stack.pop_two();

    bool to_float = lhs.var_type == VarType::F32;
    to_float |= rhs.var_type == VarType::F32;

    auto result_var_type = to_float ? VarType::F32 : VarType::I32;

    // TODO: what the fuck is this?
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

void CodeGenerator::set_cond_expr_op(CondExprOp op) {
    cond_expr_op = op;
}
