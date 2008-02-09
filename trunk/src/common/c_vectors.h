/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Vector type declarations and static inline definitions for the vector
   operations. Requires math.h to be included. */

#pragma pack(push, 4)

typedef struct {
    float x;
    float y;
} c_vec2_t;

typedef struct {
    float x;
    float y;
    float z;
} c_vec3_t;

#pragma pack(pop)

/* Vector types can be converted to float arrays and vice versa */
#define C_VEC_ARRAY(v) ((float *)&(v))

/******************************************************************************\
 Constructors.
\******************************************************************************/

static inline c_vec2_t C_vec2(float x, float y)
{
    c_vec2_t result = { x, y };
    return result;
}

static inline c_vec3_t C_vec3(float x, float y, float z)
{
    c_vec3_t result = { x, y, z };
    return result;
}

/******************************************************************************\
 Binary operators with another vector.
\******************************************************************************/

#define VEC2_OPFUNC(name, op) \
    static inline c_vec2_t C_vec2_##name(c_vec2_t a, c_vec2_t b) \
    { \
        return C_vec2(a.x op b.x, a.y op b.y); \
    }

#define VEC3_OPFUNC(name, op) \
    static inline c_vec3_t C_vec3_##name(c_vec3_t a, c_vec3_t b) \
    { \
        return C_vec3(a.x op b.x, a.y op b.y, a.z op b.z); \
    }

VEC2_OPFUNC(add, +);
VEC2_OPFUNC(sub, -);
VEC2_OPFUNC(scale, *);
VEC2_OPFUNC(invscale, /);
VEC3_OPFUNC(add, +);
VEC3_OPFUNC(sub, -);
VEC3_OPFUNC(scale, *);
VEC3_OPFUNC(invscale, /);

#undef VEC2_OPFUNC
#undef VEC3_OPFUNC

/******************************************************************************\
 Binary operators with a scalar.
\******************************************************************************/

#define VEC2_OPFUNCF(name, op) \
    static inline c_vec2_t C_vec2_##name(c_vec2_t a, float f) \
    { \
        return C_vec2(a.x op f, a.y op f); \
    }

#define VEC3_OPFUNCF(name, op) \
    static inline c_vec3_t C_vec3_##name(c_vec3_t a, float f) \
    { \
        return C_vec3(a.x op f, a.y op f, a.z op f); \
    }

VEC2_OPFUNCF(addf, +);
VEC2_OPFUNCF(subf, -);
VEC2_OPFUNCF(scalef, *);
VEC2_OPFUNCF(invscalef, /);
VEC3_OPFUNCF(addf, +);
VEC3_OPFUNCF(subf, -);
VEC3_OPFUNCF(scalef, *);
VEC3_OPFUNCF(invscalef, /);

#undef VEC2_OPFUNCF
#undef VEC3_OPFUNCF

/******************************************************************************\
 Vector dot and cross products.
\******************************************************************************/

static inline float C_vec2_dot(c_vec2_t a, c_vec2_t b)
{
    return a.x * b.x + a.y * b.y;
}

static inline float C_vec3_dot(c_vec3_t a, c_vec3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float C_vec2_cross(c_vec2_t a, c_vec2_t b)
{
    return a.y * b.x - a.x * b.y;
}

static inline c_vec3_t C_vec3_cross(c_vec3_t a, c_vec3_t b)
{
    return C_vec3(a.y * b.z - a.z * b.y,
                  a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x);
}

/******************************************************************************\
 Vector magnitude/length.
\******************************************************************************/

static inline float C_vec2_square_len(c_vec2_t p)
{
    return p.x * p.x + p.y * p.y;
}

static inline float C_vec2_len(c_vec2_t p)
{
    return sqrt(C_vec2_square_len(p));
}

static inline float C_vec3_square_len(c_vec3_t p)
{
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

static inline float C_vec3_len(c_vec3_t p)
{
    return sqrt(C_vec3_square_len(p));
}

/******************************************************************************\
 Vector normalization.
\******************************************************************************/

static inline c_vec2_t C_vec2_norm(c_vec2_t p)
{
    return C_vec2_invscalef(p, C_vec2_len(p));
}

static inline c_vec3_t C_vec3_norm(c_vec3_t p)
{
    return C_vec3_invscalef(p, C_vec3_len(p));
}

/******************************************************************************\
 Vector comparison.
\******************************************************************************/

static inline int C_vec2_eq(c_vec2_t a, c_vec2_t b)
{
    return a.x == b.x && a.y == b.y;
}

static inline int C_vec3_eq(c_vec3_t a, c_vec3_t b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/******************************************************************************\
 Vector interpolation.
\******************************************************************************/

static inline c_vec2_t C_vec2_lerp(c_vec2_t a, float lerp, c_vec2_t b)
{
        return C_vec2(a.x + lerp * (b.x - a.x), a.y + lerp * (b.y - a.y));
}

static inline c_vec3_t C_vec3_lerp(c_vec3_t a, float lerp, c_vec3_t b)
{
        return C_vec3(a.x + lerp * (b.x - a.x),
                      a.y + lerp * (b.y - a.y),
                      a.z + lerp * (b.z - a.z));
}

