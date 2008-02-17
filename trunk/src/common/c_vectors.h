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
        float x, y;
} c_vec2_t;

typedef struct {
        float x, y, z;
} c_vec3_t;

/* TODO: Are four-member vectors always colors? Quaternions anyone? */
typedef struct c_color {
        float r, g, b, a;
} c_color_t;

#pragma pack(pop)

/* Vector types can be converted to float arrays and vice versa */
#define C_ARRAYF(v) ((float *)&(v))

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

static inline c_color_t C_color(float r, float g, float b, float a)
{
    c_color_t result = { r, g, b, a };
    return result;
}

/******************************************************************************\
 Binary operators with another vector.
\******************************************************************************/
#define OPFUNC(name, op) \
    static inline c_vec2_t C_vec2_##name(c_vec2_t a, c_vec2_t b) \
    { \
        return C_vec2(a.x op b.x, a.y op b.y); \
    } \
    static inline c_vec3_t C_vec3_##name(c_vec3_t a, c_vec3_t b) \
    { \
        return C_vec3(a.x op b.x, a.y op b.y, a.z op b.z); \
    } \
    static inline c_color_t C_color_##name(c_color_t a, c_color_t b) \
    { \
        return C_color(a.r op b.r, a.g op b.g, a.b op b.b, a.a op b.a); \
    }

OPFUNC(add, +);
OPFUNC(sub, -);
OPFUNC(scale, *);
OPFUNC(invscale, /);

#undef OPFUNC

/******************************************************************************\
 Binary operators with a scalar.
\******************************************************************************/
#define OPFUNCF(name, op) \
    static inline c_vec2_t C_vec2_##name(c_vec2_t a, float f) \
    { \
        return C_vec2(a.x op f, a.y op f); \
    } \
    static inline c_vec3_t C_vec3_##name(c_vec3_t a, float f) \
    { \
        return C_vec3(a.x op f, a.y op f, a.z op f); \
    } \
    static inline c_color_t C_color_##name(c_color_t a, float f) \
    { \
        return C_color(a.r op f, a.g op f, a.b op f, a.a op f); \
    }

OPFUNCF(addf, +);
OPFUNCF(subf, -);
OPFUNCF(scalef, *);
OPFUNCF(invscalef, /);

#undef OPFUNCF

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

/******************************************************************************\
 Clamp a color to valid range.
\******************************************************************************/
static inline c_color_t C_color_clamp(c_color_t c)
{
        int i;

        for (i = 0; i < 4; i++) {
                if (C_ARRAYF(c)[i] < 0.f)
                        C_ARRAYF(c)[i] = 0.f;
                if (C_ARRAYF(c)[i] > 1.f)
                        C_ARRAYF(c)[i] = 1.f;
        }
        return c;
}

/******************************************************************************\
 Blend a color onto another color.
\******************************************************************************/
static inline c_color_t C_color_blend(c_color_t dest, c_color_t src)
{
        src.r *= src.a;
        src.g *= src.a;
        src.b *= src.a;
        return C_color_add(C_color_scalef(dest, 1.f - src.a), src);
}

/******************************************************************************\
 Construct a color from 0-255 ranged integer values.
\******************************************************************************/
static inline c_color_t C_color32(int r, int g, int b, int a)
{
    c_color_t result = { r, g, b, a };
    return C_color_invscalef(result, 255.f);
}

