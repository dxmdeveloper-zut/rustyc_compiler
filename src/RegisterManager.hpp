#pragma once
#include <cassert>
#include <string>
#include <array>

class Compiler;
class RegisterManager;

struct Reg {
public:
    enum class Type : char {
        UNASSIGNED = '\0',
        T_REG = 't',
        F_REG = 'f',
        V_REG = 'v',
        A_REG = 'a',
    };

public:
    Reg() = default;

    Reg(const std::string &reg_name);

    Reg(const char *reg_name);

    Reg(Reg &&other) noexcept;

    bool is_valid() const;

    Type get_type() const;

    std::string str() const;

    void release();

    operator std::string() const;

    explicit operator bool() const;

    Reg &operator=(Reg &&other) noexcept;

    ~Reg();

private:
    Reg(Type type, unsigned reg_index, Compiler *compiler);

    unsigned reg_index = 0;
    Type type = Type::UNASSIGNED;
    Compiler *compiler = nullptr;

    friend class RegisterManager;
};

inline std::ostream &operator <<(std::ostream &os, const Reg &reg) {
    return os << reg.str();
}

class RegisterManager {
public:
    static constexpr int T_REG_COUNT = 8;
    static constexpr int F_REG_COUNT = 32;
    static constexpr int V_REG_COUNT = 2;
    static constexpr int A_REG_COUNT = 4;

    enum class StoringType {
        NONE = 0,
        TEMP = 1,
        CALC_RESULT = 2,
        VARIABLE = 3,
        RESERVED = 4,
    };

    explicit RegisterManager(Compiler *compiler);

    [[nodiscard]] int set_storing_type(const Reg &reg, StoringType type);

    void tmp_cleanup();

    void release_calc_results();

    void release_register(Reg::Type type, int idx);

    Reg get_free_register(Reg::Type type, StoringType purpose = StoringType::TEMP);

private:
    struct RegStorage {
        StoringType storing_type = StoringType::NONE;
        std::string var_id{};

        void reset();
    };

    std::array<RegStorage, T_REG_COUNT> t_regs{};
    std::array<RegStorage, F_REG_COUNT> f_regs{};
    Compiler *compiler = nullptr;
};
