#pragma once

#include "Bodies.hpp"

void compute_collisions_cpu(Bodies& bodies);

void compute_gravity_cpu(Bodies& bodies, float G);

void compute_gravity_cpu_parallel(Bodies& bodies, float G, size_t thread_count);
