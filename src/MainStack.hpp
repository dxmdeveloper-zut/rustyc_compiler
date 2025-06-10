#pragma once
#include <stack>
#include <utility>
#include "enums.hpp"
#include "HashMap.hpp"

class MainStack {
public:
    explicit MainStack(HashMap<std::string, SymbolInfo> &symbolTable);

    void push(ExprElemType type, const std::string &value, VarType var_type = VarType::UNDEFINED);

    void push(const StackEntry &entry);

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