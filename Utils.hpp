#pragma once

#include <chrono>
#include <type_traits>
#include <functional>
#include <glm/glm.hpp>

template<typename T>
T rand_0_1();

template<typename T>
T rand_1_1();

template<> struct std::hash<glm::ivec2> {
    size_t operator()(const glm::ivec2& v) const {
        union {
            struct { int i_a; int i_b; } i;
            size_t s;
        } u{.i = {v.x, v.y}};
        return u.s;
    }
};

template<typename T>
std::enable_if_t<std::is_integral_v<T>, T> div_ceil(T a, T b) {
    return (a + b - 1) / b;
}

template<typename T>
T max(T t) {
    return t;
}

template<typename T, typename ...Ts>
T max(T t, const Ts... ts) {
    return std::max(t, max(ts...));
}

template<typename V>
struct UniquePairs {
    V& view;
    using v_it_t = decltype(view.begin());
    using val_t = decltype(*view.begin());
    UniquePairs(V& v) : view(v) {}

    struct it {
        v_it_t begin;
        v_it_t first;
        v_it_t second;

        it operator++() {
            ++first;
            if(first == second) {
                ++second;
                first = begin;
            }
            return *this;
        }
        it operator++(int) {
            auto it = *this;
            ++(*this);
            return it;
        }
        std::tuple<val_t, val_t> operator*() {
            return std::tuple<val_t, val_t>(*first, *second);
        }
        bool operator!=(it& other) {
            return second != other.second || first != other.first;
        }
    };

    it begin() {
        if(view.begin() == view.end())
            return end();
        return it{view.begin(), view.begin(), std::next(view.begin())};
    }
    it end() { return it{view.begin(), view.begin(), view.end()}; }

    size_t size() {
        auto view_size = view.size();
        return (view_size - 1) * view_size / 2;
    }

    it it_at(size_t id) {
        if(id >= size())
            return end();
        size_t row = sqrt(8.0*id + 1) / 2 + 0.5;
        size_t col = id - (row - 1) * row / 2;
        return {view.begin(), std::next(view.begin(), col), std::next(view.begin(), row)};
    }
};

template<typename IT>
struct SimpleRange {
    IT _begin;
    IT _end;
    SimpleRange(IT begin, IT end)
        : _begin(begin)
        , _end(end) {}
    IT begin() { return _begin; }
    IT end() { return _end; }
};

template<typename duration_t>
struct Timer {
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point lap_start_time;

    Timer() {
        start();
    }

    void start() {
        start_time = std::chrono::steady_clock::now();
        lap_start_time = start_time;
    }

    duration_t cast(std::chrono::steady_clock::duration d) {
        return std::chrono::duration_cast<duration_t>(d);
    }

    duration_t lap() {
        auto now = std::chrono::steady_clock::now();
        auto lap_time = now - lap_start_time;
        lap_start_time = now;
        return cast(lap_time);
    }
    duration_t elapsed() {
        return cast(std::chrono::steady_clock::now() - start_time);
    }
};
