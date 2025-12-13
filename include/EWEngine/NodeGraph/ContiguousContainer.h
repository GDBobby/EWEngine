#pragma once

#include <concepts>
#include <array>
#include <cstdint>

namespace EWE{
    template<typename T>
    requires std::assignable_from<T&, T&&>
    struct ContiguousContainer{
        //id like an array but i dont feel like dealing with objects that cant be trivially constructed
        std::vector<T> data;
        using value_type = T;
        using size_type  = std::size_t;

        ContiguousContainer() = default;
        explicit ContiguousContainer(size_type n) : data(n) {}
        ContiguousContainer(std::initializer_list<T> init_list) : data(init_list) {}
        
        auto begin() noexcept { return data.begin(); }
        auto end()   noexcept { return data.end(); }
        auto begin() const noexcept { return data.begin(); }
        auto end()   const noexcept { return data.end(); }

        bool empty() const noexcept { return data.empty(); }
        size_type size() const noexcept { return data.size(); }
        size_type capacity() const noexcept { return data.capacity(); }
        void reserve(size_type n) { data.reserve(n); }
        //void shrink_to_fit() { data.shrink_to_fit(); }
        
        T& operator[](size_type i) noexcept { return data[i]; }
        const T& operator[](size_type i) const noexcept { return data[i]; }

        T& at(size_type i) { return data.at(i); }
        const T& at(size_type i) const { return data.at(i); }

        T& front() noexcept { return data.front(); }
        T& back()  noexcept { return data.back(); }

        void clear() noexcept { data.clear(); }

        template<class... Args>
        requires std::constructible_from<T, Args&&...>
        T& emplace_back(Args&&... args) {
            return data.emplace_back(std::forward<Args>(args)...);
        }

        void push_back(T value) {
            data.push_back(std::move(value));
        }
        void erase(size_type index) {
            if (index != data.size() - 1) {
                data[index] = std::move(data.back());
            }
            data.pop_back();
        }
        void pop_back() {
            data.pop_back();
        }

    };
}