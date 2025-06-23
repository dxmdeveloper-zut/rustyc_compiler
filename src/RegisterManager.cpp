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

Reg::Reg(Reg &other) : reg_index(other.reg_index), type(other.type), compiler(nullptr) {
};

Reg::Reg(Reg &&other) noexcept
: reg_index(other.reg_index), type(other.type), compiler(other.compiler),
reserved_for_calc_result_alloc(other.reserved_for_calc_result_alloc){
    other.compiler = nullptr;
}

void Reg::reset() {
    reg_index = 0;
    type = Type::UNASSIGNED;
    compiler = nullptr;
}

bool Reg::is_valid() const {
    return type != Type::UNASSIGNED;
}

Reg::Type Reg::get_type() const {
    return type;
}

std::string Reg::str() const {
    assert(type != Type::UNASSIGNED);
    std::string str = std::string("$") + char(type) + std::to_string(reg_index);
    return str;
}

Reg::operator bool() const {
    return type != Type::UNASSIGNED;
}

Reg & Reg::operator=(Reg &&other) noexcept {
    this->compiler = other.compiler;
    this->type = other.type;
    this->reg_index = other.reg_index;
    this->reserved_for_calc_result_alloc = other.reserved_for_calc_result_alloc;
    other.compiler = nullptr;
    return *this;
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

bool Reg::is_reserved_for_calc_result_storing() const {
    return reserved_for_calc_result_alloc;
}

Reg::Reg(Type type, unsigned reg_index, Compiler *compiler): type {type}, reg_index{reg_index}, compiler{compiler} {}

RegisterManager::RegisterManager(Compiler *compiler): compiler(compiler) {
    assert(compiler != nullptr);
    f_regs[12] = {StoringType::RESERVED};
}

int RegisterManager::try_preserve_value(Reg &reg, StoringType type, const std::string &symbol_name) {
    assert(reg && type != StoringType::NONE);
    reg.reserved_for_calc_result_alloc = false;

    if (reg.compiler == nullptr)
        return 0; // for copied Regs from symbolTable symbols

    if (reg.get_type() == Reg::Type::T_REG) {
        auto count = std::count_if(t_regs.begin(), t_regs.end(),
            [](const RegStorage &r) { return r.storing_type == StoringType::NONE; });

        if (count > 2) {
            t_regs[reg.reg_index].storing_type = type;
            t_regs[reg.reg_index].var_id = symbol_name;
            return 0;
        }
        return -1;
    }
    if (reg.get_type() == Reg::Type::F_REG) {
        auto count = std::count_if(f_regs.begin(), f_regs.end(),
            [](const RegStorage &r) { return r.storing_type == StoringType::NONE; });

        if (count > 2) {
            f_regs[reg.reg_index].storing_type = type;
            f_regs[reg.reg_index].var_id = symbol_name;
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
    for (auto &reg_storage: t_regs) {
        if (reg_storage.storing_type == StoringType::CALC_RESULT) {
            if (!reg_storage.var_id.empty()) {
                compiler->symbolTable.at(reg_storage.var_id).occupied_reg.reset();
            }
            reg_storage.reset();

        }
    }
    for (auto &reg_storage: f_regs) {
        if (reg_storage.storing_type == StoringType::CALC_RESULT) {
            if (!reg_storage.var_id.empty()) {
                compiler->symbolTable.at(reg_storage.var_id).occupied_reg.reset();
            }
            reg_storage.reset();
        }
    }
}

void RegisterManager::gen_dump_calc_results_to_memory(std::ostream &text_region) {
    auto lambda = [&](RegStorage &reg_storage, const std::string& postfix) {
        if (reg_storage.storing_type == StoringType::CALC_RESULT) {
            if (!reg_storage.var_id.empty()) {
                Reg *reg_ptr = &compiler->symbolTable.at(reg_storage.var_id).occupied_reg;
                text_region << "s" << postfix << " " << *reg_ptr << ", " << reg_storage.var_id << std::endl;
                reg_ptr->reset();
                reg_storage.reset();
            }
        }
    };
    for (auto &reg_storage: t_regs) {
        lambda(reg_storage, "w");
    }
    for (auto &reg_storage: f_regs) {
        lambda(reg_storage, ".s");
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

Reg RegisterManager::get_free_register(Reg::Type type, StoringType target_purpose) {
    assert(type == Reg::Type::T_REG || type == Reg::Type::F_REG);

    Reg void_reg(Reg::Type::UNASSIGNED, 0, nullptr);

    auto get_free_reg = [&](auto &regs, Reg::Type reg_type) -> Reg {

        // Find first free register
        if (target_purpose == StoringType::TEMP) {
            for (unsigned i = 0; i < regs.size(); i++) {
                if (regs[i].storing_type == StoringType::NONE) {
                    regs[i].storing_type = StoringType::TEMP;
                    return {reg_type, i, compiler};
                }
            }
        } else {
            for (unsigned i = regs.size(); i-- > 0;) {
                if (regs[i].storing_type == StoringType::NONE) {
                    regs[i].storing_type = StoringType::TEMP;
                    Reg r = {reg_type, i, compiler};
                    if (target_purpose == StoringType::CALC_RESULT)
                        r.reserved_for_calc_result_alloc = true;
                    return std::move(r);
                }
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
