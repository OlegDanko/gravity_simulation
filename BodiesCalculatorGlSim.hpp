#pragma once

#include "Bodies.hpp"
#include "GLSimShaders.hpp"
#include <functional>
#include <tuple>
#include <array>
#include <iostream>
#include <algorithm>

template<typename BUF_t>
struct cleaner {
    template<typename T>
    void operator()(BUF_t& buf, T val = {}) {
        std::fill(buf.begin(), buf.end(), val);
    }
};

template<typename T>
struct cleaner<std::vector<std::vector<T>>> {
    void operator()(std::vector<std::vector<T>>& buf, T val = {}) {
        for(auto& b : buf) {
            cleaner<std::vector<T>>()(b, val);
        }
    };
};

template<typename T>
struct resizer {
    static void resize(T& t, size_t size) {
        t.resize(size);
    }
};
template<typename T>
struct resizer<std::vector<std::vector<T>>> {
    template<typename ... Ts>
    static void resize(std::vector<std::vector<T>>& t, size_t size, Ts ... ts) {
        t.resize(size);
        for(auto & v : t) resizer<std::vector<T>>::resize(v, ts...);
    }
};

template<typename BUF_t>
struct TexDoubleBuffer {
    BUF_t read;
    BUF_t write;
    void swap() {
        std::swap(read, write);
    }
    template<typename T>
    void clear(T val = {}) {
        cleaner<BUF_t>()(read, val);
        cleaner<BUF_t>()(write, val);
    }
};

class BodiesCalculatorGlSim
{
    Bodies& bodies;

    struct Chunk {
        TexDoubleBuffer<position_tex_t> position;
        TexDoubleBuffer<velocity_tex_t> velocity;
        TexDoubleBuffer<mass_tex_t> mass;
    };

    std::vector<Chunk> chunks;
    delta_v_matrix_t velocities_delta;
    forces_tile_t forces_tile;

    size_t chunk_size{64};


    std::vector<size_t> elements;
    std::vector<std::pair<size_t, size_t>> elements_pairs;

    struct copy_from {
        template<typename IT>
        static void cp(IT& a, IT& b) {
            *a++ = *b++;
        }
    };
    struct copy_to {
        template<typename IT>
        static void cp(IT& a, IT& b) {
            *b++ = *a++;
        }
    };

    template<typename CP>
    void copy_bodies() {
        auto pos_in_it = bodies.get_positions().begin();
        auto vel_in_it = bodies.get_velocities().begin();
        auto mass_in_it = bodies.get_masses().begin();

        for(auto& chunk : chunks) {
            auto pos_out_it = chunk.position.read.begin();
            auto vel_out_it = chunk.velocity.read.begin();
            auto mass_out_it = chunk.mass.read.begin();

            while(pos_out_it != chunk.position.read.end()) {
                if(pos_in_it == bodies.get_positions().end())
                    return;

                CP::cp(pos_out_it, pos_in_it);
                CP::cp(vel_out_it, vel_in_it);
                CP::cp(mass_out_it, mass_in_it);
            }
        }
    }

public:
    BodiesCalculatorGlSim(Bodies& bodies);

    void copy_from_bodies();
    void copy_to_bodies();

    void update();
};

