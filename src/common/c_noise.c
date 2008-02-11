/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file implements Perlin noise. */

#include "c_shared.h"
#include <stdlib.h>

static unsigned char map[256];
static c_vec3_t grad[256];

/* TODO: Use something other than rand(). We can't expect it to be
   consistent across platforms.

   Also TODO: figure out why my noise function isn't giving anything
   beyond about \pm 0.4. It should be in [-1, 1]. */

/******************************************************************************\
 Generate a random floating number in [0, 1)
\******************************************************************************/
static float C_rand()
{
        return rand() / (RAND_MAX + 1.0);
}

/******************************************************************************\
 Seed the PRNG and generate the gradients. Should come out the same every time
 for the same seed.
\******************************************************************************/
void C_noise3_seed(unsigned int seed)
{
        int i;

        srand(seed);

        /* Fill mapping sequentially */
        for (i = 0; i < 256; i++)
                map[i] = (unsigned char)i;

        /* Knuth shuffle it. */
        for (i = 0; i < 256; i++) {
                int j;
                unsigned char tmp;

                j = i + (int)(C_rand() * 256 - i);
                tmp = map[i];
                map[i] = map[j];
                map[j] = tmp;
        }

        /* Fill in random gradients */
        for (i = 0; i < 256; i++) {
                grad[i] = C_vec3(C_rand(), C_rand(), C_rand());

                /* Avoid divide-by-zero */
                while (grad[i].x == 0.0 && grad[i].y == 0.0 && grad[i].z == 0)
                        grad[i] = C_vec3(C_rand(), C_rand(), C_rand());

                grad[i] = C_vec3_norm(grad[i]);
        }
}

/******************************************************************************\
 Linear interpolation
\******************************************************************************/
static inline float lerp(float a, float b, float t)
{
        return a + t * (b - a);
}

/******************************************************************************\
 "Ease curve". Apparently this makes the noise nicer somehow.
\******************************************************************************/
static inline float s_curve(float x)
{
        return x * x * (3 - 2 * x);
}

/******************************************************************************\
 Calculate 3D Perlin noise. Nodes are numbered like this:

     6----5
    /    /|
   2----3 |
   |    | 4
   |    |/
   0----1
\******************************************************************************/
float C_noise3(float x, float y, float z)
{
        c_vec3_t pt, t, nodes[8];
        float temps[3], node_vals[8];
        int i;

        pt = C_vec3(x, y, z);
        nodes[0] = C_vec3(floor(x), floor(y), floor(z));
        nodes[1] = C_vec3(floor(x + 1), floor(y), floor(z));
        nodes[2] = C_vec3(floor(x), floor(y + 1), floor(z));
        nodes[3] = C_vec3(floor(x + 1), floor(y + 1), floor(z));
        nodes[4] = C_vec3(floor(x + 1), floor(y), floor(z + 1));
        nodes[5] = C_vec3(floor(x + 1), floor(y + 1), floor(z + 1));
        nodes[6] = C_vec3(floor(x), floor(y + 1), floor(z + 1));
        nodes[7] = C_vec3(floor(x), floor(y), floor(z + 1));

        for (i = 0; i < 8; i++) {
                int grad_ind;

                grad_ind = map[(int)nodes[i].z & 0xff];
                grad_ind = map[((int)nodes[i].y + grad_ind) & 0xff];
                grad_ind = ((int)nodes[i].x + grad_ind) & 0xff;
                node_vals[i] = C_vec3_dot(C_vec3_sub(pt, nodes[i]),
                                          grad[grad_ind]);
        }

        /* Weighted averages FTW */
        t = C_vec3(s_curve(x - floor(x)),
                   s_curve(y - floor(y)),
                   s_curve(z - floor(z)));

        /* Average 2 and 6, 3 and 5 in z, and then average results in x */
        temps[0] = lerp(node_vals[2], node_vals[6], t.z);
        temps[1] = lerp(node_vals[3], node_vals[5], t.z);
        temps[0] = lerp(temps[0], temps[1], t.x);

        /* Average 0 and 7, 1 and 4 in z and average results in x */
        temps[1] = lerp(node_vals[0], node_vals[7], t.z);
        temps[2] = lerp(node_vals[1], node_vals[4], t.z);
        temps[1] = lerp(temps[1], temps[2], t.x);

        /* Return the weighted average of those results in y
           Remember that temps[0] is above temps[1]. */
        return lerp(temps[1], temps[0], t.y);
}

/******************************************************************************\
 Calculate 3D Perlin noise. Nodes are numbered like this:
\******************************************************************************/
float C_noise3_fractal(int levels, float x, float y, float z)
{
        float factor, sum;
        int i;

        sum = 0.0;
        for (i = 0, factor = 1.0; i < levels; i++, factor *= 2)
                sum += C_noise3(x * factor, y * factor, z * factor) / factor;

        return sum;
}
