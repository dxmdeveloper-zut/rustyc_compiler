#include "Compiler.hpp"
#include "RegisterManager.hpp"

#include <algorithm>

Reg::Reg(const std::string& reg_name) : Reg(reg_name.c_str()){};

Reg::Reg(const char *reg_name): reg_index(0), type(Type::T_REG), compiler(nullptr) {
    assert(reg_name[0] == '$');
    assert(strchr("tfva", reg_name[1]) != nullptr);

    type = static_cast<Type>(reg_name[1]);
    reg_index = std::stoi(&reg_name[2]);

    assert((type == Type::T_REG && reg_index < RegisterManager::T_REG_COUNT)
        || (type == Type::F_REG && reg_index < RegisterManager::F_REG_COUNT)
        || (type == Type::V_REG && reg_index < RegisterManager::V_REG_COUNT)
        || (type == Type::A_REG && reg_index < RegisterManager::A_REG_COUNT));
}

Reg::Reg(Reg &&other) noexcept: reg_index(other.reg_index), type(other.type), compiler(other.compiler) {
    assert(other.type != Type::UNASSIGNED);
    other.compiler = nullptr;
}

bool Reg::is_valid() const {
    return type != Type::UNASSIGNED;
}

Reg::Type Reg::get_type() const {
    return type;
}

std::string Reg::str() const {
    assert(type != Type::UNASSIGNED);
    return "$" + char(type) + std::to_string(reg_index);
}

Reg::operator std::string() const {
    return str();
}

Reg::operator bool() const {
    return type != Type::UNASSIGNED;
}

Reg & Reg::operator=(Reg &&other) noexcept {
    assert(other.type != Type::UNASSIGNED);

    this->compiler = other.compiler;
    this->type = other.type;
    this->reg_index = other.reg_index;
    other.compiler = nullptr;
}

void Reg::release() {
    if (compiler) {
        compiler->reg_mgr.release_register(type, reg_index);
    }
    compiler = nullptr;
}

Reg::~Reg() {
    release();
}

Reg::Reg(Type type, unsigned reg_index, Compiler *compiler): type {type}, reg_index{reg_index}, compiler{compiler} {}

RegisterManager::RegisterManager(Compiler *compiler): compiler(compiler) {
    assert(compiler != nullptr);
    f_regs[12] = {StoringType::RESERVED};
}

int RegisterManager::set_storing_type(const Reg &reg, StoringType type) {
    assert(reg && type != StoringType::NONE);

    if (reg.get_type() == Reg::Type::T_REG) {
        auto count = std::count_if(t_regs.begin(), t_regs.end(),
            [](const RegStorage &r) { return r.storing_type == StoringType::NONE; });

        if (count > 2) {
            t_regs[reg.reg_index].storing_type = type;
            return 0;
        }
        return -1;
    }
    if (reg.get_type() == Reg::Type::F_REG) {
        auto count = std::count_if(f_regs.begin(), f_regs.end(),
            [](const RegStorage &r) { return r.storing_type == StoringType::NONE; });

        if (count > 2) {
            f_regs[reg.reg_index].storing_type = type;
            return 0;
        }
        return -1;
    }
    return -1;
}

void RegisterManager::tmp_cleanup() {
    for (auto &reg: t_regs) {
        if (reg.storing_type == StoringType::TEMP)
            reg.reset();
    }
    for (auto &reg: f_regs) {
        if (reg.storing_type == StoringType::TEMP)
            reg.reset();
    }
}

void RegisterManager::release_calc_results() {
    for (auto &reg: t_regs) {
        if (reg.storing_type == StoringType::CALC_RESULT)
            reg.reset();
    }
    for (auto &reg: f_regs) {
        if (reg.storing_type == StoringType::CALC_RESULT)
            reg.reset();
    }
}

void RegisterManager::release_register(Reg::Type type, int idx) {
    // TODO: variable save
    if (type == Reg::Type::T_REG) {
        t_regs[idx].reset();
    } else if (type == Reg::Type::F_REG) {
        f_regs[idx].reset();
    }
}

Reg RegisterManager::get_free_register(Reg::Type type, StoringType purpose) {
    assert(type == Reg::Type::T_REG || type == Reg::Type::F_REG);

    Reg void_reg(Reg::Type::UNASSIGNED, 0, nullptr);

    auto get_free_reg = [&](auto &regs, Reg::Type reg_type) -> Reg {
        // Count free registers
        auto count = std::count_if(regs.begin(), regs.end(),
            [](const RegStorage &reg) { return reg.storing_type == StoringType::NONE; });

        if ((purpose != StoringType::TEMP && count < 2) || count == 0) {
            // Try to release temporary registers
            tmp_cleanup();
            count = std::count_if(regs.begin(), regs.end(),
                [](const RegStorage &reg) { return reg.storing_type == StoringType::NONE; });

            if ((purpose != StoringType::TEMP && count < 2) || count == 0)
                return void_reg;
        }
        // Find first free register
        for (unsigned i = 0; i < regs.size(); i++) {
            if (regs[i].storing_type == StoringType::NONE) {
                regs[i].storing_type = purpose;
                return {reg_type, i, compiler};
            }
        }
        return void_reg;
    };

    if (type == Reg::Type::T_REG)
        return get_free_reg(t_regs, type);

    if (type == Reg::Type::F_REG)
        return get_free_reg(f_regs, type);

    return void_reg;
}

void RegisterManager::RegStorage::reset() {
    storing_type = StoringType::NONE;
    var_id.clear();
}
