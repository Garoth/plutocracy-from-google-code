/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include <math.h>

#pragma pack(push, 4)

typedef struct {
    float x;
    float y;
} c_pt2_t;

typedef struct {
    float x;
    float y;
    float z;
} c_pt3_t;

#pragma pack(pop)

/* pt2 functions */
static inline c_pt2_t C_pt2(float x, float y)
{
    c_pt2_t result = { x, y };
    return result;
}

#define PT2_OPFUNC(name, op)                                    \
    static inline c_pt2_t C_pt2_##name(c_pt2_t a, c_pt2_t b)    \
    {                                                           \
        return C_pt2(a.x op b.x, a.y op b.y);                   \
    }

PT2_OPFUNC(add, +);
PT2_OPFUNC(sub, -);
PT2_OPFUNC(scale, *);
PT2_OPFUNC(invscale, /);

#define PT2_OPFUNCF(name, op)                                 \
    static inline c_pt2_t C_pt2_##name(c_pt2_t a, float f)    \
    {                                                         \
        return C_pt2(a.x op f, a.y op f);                     \
    }

PT2_OPFUNCF(scalef, *);
PT2_OPFUNCF(invscalef, /);

static inline float C_pt2_dot(c_pt2_t a, c_pt2_t b)
{
    return a.x * b.x + a.y * b.y;
}

static inline float C_pt2_square_len(c_pt2_t p)
{
    return p.x * p.x + p.y * p.y;
}

static inline float C_pt2_len(c_pt2_t p)
{
    return sqrt(C_pt2_square_len(p));
}

static inline c_pt2_t C_pt2_normalize(c_pt2_t p)
{
    return C_pt2_invscalef(p, C_pt2_len(p));
}

static inline int C_pt2_eq(c_pt2_t a, c_pt2_t b)
{
    return a.x == b.x && a.y == b.y;
}

#undef PT2_OPFUNC
#undef PT2_OPFUNCF

/* pt3 functions */
static inline c_pt3_t C_pt3(float x, float y, float z)
{
    c_pt3_t result = { x, y, z };
    return result;
}

#define PT3_OPFUNC(name, op)                                    \
    static inline c_pt3_t C_pt3_##name(c_pt3_t a, c_pt3_t b)    \
    {                                                           \
        return C_pt3(a.x op b.x, a.y op b.y, a.z op b.z);       \
    }

PT3_OPFUNC(add, +);
PT3_OPFUNC(sub, -);
PT3_OPFUNC(scale, *);
PT3_OPFUNC(invscale, /);

#define PT3_OPFUNCF(name, op)                                 \
    static inline c_pt3_t C_pt3_##name(c_pt3_t a, float f)    \
    {                                                         \
        return C_pt3(a.x op f, a.y op f, a.z op f);           \
    }

PT3_OPFUNCF(scalef, *);
PT3_OPFUNCF(invscalef, /);

static inline float C_pt3_dot(c_pt3_t a, c_pt3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline c_pt3_t C_pt3_cross(c_pt3_t a, c_pt3_t b)
{
    return C_pt3(a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x);
}

static inline float C_pt3_square_len(c_pt3_t p)
{
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

static inline float C_pt3_len(c_pt3_t p)
{
    return sqrt(C_pt3_square_len(p));
}

static inline c_pt3_t C_pt3_normalize(c_pt3_t p)
{
    return C_pt3_invscalef(p, C_pt3_len(p));
}

static inline int C_pt3_eq(c_pt3_t a, c_pt3_t b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

#undef PT3_OPFUNC
#undef PT3_OPFUNCF
