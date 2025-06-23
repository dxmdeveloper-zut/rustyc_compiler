#pragma once
#include <cstdint>
#include <unordered_map>

template<typename TKey, typename TValue>
class HashMap {
public:
    TValue &at(const TKey &key) {
        return map.at(key);
    }

    const TValue &at(const TKey &key) const {
        return map.at(key);
    }

    bool contains(const TKey &key) const {
        return map.find(key) != map.end();
    }

    TValue &operator [] (const TKey &key) {
        return map[key];
    }

    const TValue & operator [] (const TKey &key) const {
        return map[key];
    }

    std::optional<TValue*> find(const TKey &key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

    void insert(const TKey &key, TValue &&value) {
        map.insert(std::make_pair(key, std::move(value)));
    }

    void erase(const TKey &key) {
        map.erase(key);
    }

    size_t size() const {
        return map.size();
    }

    bool empty() const {
        return map.empty();
    }

    void clear() {
        map.clear();
    }

    typename std::unordered_map<TKey, TValue>::iterator begin() { return map.begin(); }
    typename std::unordered_map<TKey, TValue>::iterator end() { return map.end(); }
    typename std::unordered_map<TKey, TValue>::const_iterator begin() const { return map.begin(); }
    typename std::unordered_map<TKey, TValue>::const_iterator end() const { return map.end(); }

private:
    std::unordered_map<TKey, TValue> map;
};
