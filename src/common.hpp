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

#include "RegisterManager.hpp"


enum class ExprElemType {
    ID,
    NUMBER,
    ARRAY_ELEM,
    STRING_LITERAL, // only as argument to push it on the stack
};


enum class VarType {
    UNDEFINED,
    I32,
    F32,
    U8_ARR,
    I32_ARR,
    F32_ARR,
};

inline bool VarType_is_num(VarType type) {
    return type == VarType::I32 || type == VarType::F32;
}

inline bool VarType_is_string(VarType type) {
    return type == VarType::U8_ARR;
}

inline bool VarType_is_num_array(VarType type) {
    return type == VarType::I32_ARR || type == VarType::F32_ARR;
}

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
    bool is_arr_elem = false;

    int arr_static_idx = -1; // -1 array index has to be calculated in runtime
    std::vector<int> arr_indices; // -1 indicates element on the stack

    std::string get_instr_postfix(bool load_addresses=false) const {
        if (var_type == VarType::U8_ARR || (load_addresses && is_arr_elem))
            return "a";

        if (type == ExprElemType::ID)
            return var_type == VarType::F32 ? ".s" : "w";

        return var_type == VarType::F32 ? ".s" : "i";

    }
    bool is_literal_i32() const {
        return type == ExprElemType::NUMBER && var_type == VarType::I32;
    }
};

struct SymbolInfo {
    VarType type;
    bool temporary = false;
    std::string initial_value;
    bool tmp_in_data_region = false; // only for temporary variables
    std::vector<int> array_dims;
    std::vector<int> array_sizes;
    bool initialized = false;
    Reg occupied_reg{};
};

template<class T>
void clear_stack(std::stack<T> &stack) {
    while (!stack.empty()) {
        stack.pop();
    }
}

/// @brief Warning: this function clears the stack
template<class T>
std::vector<T> stack_dump_to_vector(std::stack<T> &stack_to_empty) {
    std::vector<T> vec;
    vec.reserve(stack_to_empty.size());
    while (!stack_to_empty.empty()) {
        vec.push_back(stack_to_empty.top());
        stack_to_empty.pop();
    }
    return vec;
}

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

