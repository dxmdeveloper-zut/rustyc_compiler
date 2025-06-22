#include "MainStack.hpp"

#include <cassert>

MainStack::MainStack(HashMap<std::string, SymbolInfo> &symbolTable): symbolTable(symbolTable) {}

void MainStack::push(ExprElemType type, const std::string &value, VarType var_type) {
    if (type == ExprElemType::STRING_LITERAL) {
        push_string_literal(value);
        return;
    }

    if (type == ExprElemType::ID && var_type == VarType::UNDEFINED) {
        auto sym = symbolTable.find(value);
        if (sym.has_value())
            var_type = sym.value()->type;
        else
            throw std::runtime_error("Use of undeclared variable: " + value);
    }
    else if (type == ExprElemType::NUMBER && var_type == VarType::F32) {
        push_float_literal(value);
        return;
    }

    stack.push({value, type, var_type});
}

void MainStack::push_id(const std::string &id, bool allow_undeclared) {
    if (allow_undeclared) {
        stack.push({id, ExprElemType::ID});
        return;
    }
    push(ExprElemType::ID, id, VarType::UNDEFINED);
}

void MainStack::push(const StackEntry &entry) {
    stack.push(entry);
}

StackEntry & MainStack::top() {
    return stack.top();
}

const StackEntry & MainStack::top() const {
    return stack.top();
}

StackEntry MainStack::pop() {
    if (stack.empty()) {
        throw std::runtime_error("Stack is empty");
    }
    StackEntry entry = stack.top();
    stack.pop();
    return entry;
}

std::pair<StackEntry, StackEntry> MainStack::pop_two() {
    auto lhs = pop();
    auto rhs = pop();
    return {lhs, rhs};
}

void MainStack::push_string_literal(const std::string& value) {
    std::string symbol_name = "__str" + std::to_string(str_counter);
    symbolTable[symbol_name] = {VarType::U8_ARR, false, value};
    stack.push({symbol_name, ExprElemType::ID, VarType::U8_ARR});
    str_counter++;
}

void MainStack::push_float_literal(const std::string &value) {
    std::string symbol_name = "__float" + std::to_string(str_counter);
    symbolTable[symbol_name] = {VarType::F32, false, value};
    stack.push({symbol_name, ExprElemType::ID, VarType::F32});
    str_counter++;
}
