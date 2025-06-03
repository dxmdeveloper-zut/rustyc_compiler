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

    std::string get_instr_postfix() const {
        if (var_type == VarType::U8_ARR)
            return "a";

        if (type == ExprElemType::ID)
            return var_type == VarType::F32 ? ".s" : "w";

        return var_type == VarType::F32 ? ".s" : "i";

    }
};

struct SymbolInfo {
    VarType type;
    bool temporary;
    std::string initial_value;
    bool initialized = false;
};


inline void dbg_print_stack(std::stack<StackEntry> stack) {
    std::cerr << std::endl << "Stack size: " << stack.size() << std::endl;

    std::vector<StackEntry> tmp_stack(stack.size());
    std::size_t i = 0;
    while (stack.size() > 0) {
        tmp_stack[i++] = stack.top();
        stack.pop();
    }
    for (const auto &entry: tmp_stack) {
        std::cerr << entry.value << std::endl;
        stack.push(entry);
    }
    std::cerr << std::endl;
}

