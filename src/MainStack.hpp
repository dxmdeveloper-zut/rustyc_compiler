#pragma once
#include <stack>
#include <utility>
#include "enums.hpp"
#include "HashMap.hpp"

class MainStack {
public:
    explicit MainStack(HashMap<std::string, SymbolInfo> &symbolTable);

    void push(ExprElemType type, const std::string &value, VarType var_type = VarType::UNDEFINED);

    void push_id(const std::string &id, bool allow_undeclared=false);

    void push(const StackEntry &entry);

    void push(int32_t value) {
        push(ExprElemType::NUMBER, std::to_string(value), VarType::I32);
    }
    void push(float value) {
        push(ExprElemType::NUMBER, std::to_string(value), VarType::F32);
    }

    StackEntry &top();

    const StackEntry &top() const;

    StackEntry pop();

    /// @return [lhs, rhs]
    std::pair<StackEntry, StackEntry> pop_two();

private:
    void push_string_literal(const std::string &value);

    void push_float_literal(const std::string &value);

private:
    std::stack<StackEntry> stack;
    HashMap<std::string, SymbolInfo> &symbolTable;
    int str_counter = 0;
    int float_counter = 0;
};