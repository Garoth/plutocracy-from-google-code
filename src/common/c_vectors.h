/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

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
 Component-wise binary operators with another vector.
\******************************************************************************/
static inline c_vec2_t C_vec2_add(c_vec2_t a, c_vec2_t b)
{
        return C_vec2(a.x + b.x, a.y + b.y);
}

static inline c_vec2_t C_vec2_sub(c_vec2_t a, c_vec2_t b)
{
        return C_vec2(a.x - b.x, a.y - b.y);
}

static inline c_vec2_t C_vec2_scale(c_vec2_t a, c_vec2_t b)
{
        return C_vec2(a.x * b.x, a.y * b.y);
}

static inline c_vec2_t C_vec2_div(c_vec2_t a, c_vec2_t b)
{
        return C_vec2(a.x / b.x, a.y / b.y);
}

static inline c_vec3_t C_vec3_add(c_vec3_t a, c_vec3_t b)
{
        return C_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline c_vec3_t C_vec3_sub(c_vec3_t a, c_vec3_t b)
{
        return C_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline c_vec3_t C_vec3_scale(c_vec3_t a, c_vec3_t b)
{
        return C_vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

static inline c_vec3_t C_vec3_div(c_vec3_t a, c_vec3_t b)
{
        return C_vec3(a.x / b.x, a.y / b.y, a.z / b.z);
}

static inline c_color_t C_color_add(c_color_t a, c_color_t b)
{
        return C_color(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}

static inline c_color_t C_color_sub(c_color_t a, c_color_t b)
{
        return C_color(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
}

static inline c_color_t C_color_scale(c_color_t a, c_color_t b)
{
        return C_color(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

static inline c_color_t C_color_div(c_color_t a, c_color_t b)
{
        return C_color(a.r / b.r, a.g / b.g, a.b / b.b, a.a / b.a);
}

/******************************************************************************\
 Binary operators with a scalar.
\******************************************************************************/
static inline c_vec2_t C_vec2_addf(c_vec2_t a, float f)
{
        return C_vec2(a.x + f, a.y + f);
}

static inline c_vec2_t C_vec2_subf(c_vec2_t a, float f)
{
        return C_vec2(a.x - f, a.y - f);
}

static inline c_vec2_t C_vec2_scalef(c_vec2_t a, float f)
{
        return C_vec2(a.x * f, a.y * f);
}

static inline c_vec2_t C_vec2_divf(c_vec2_t a, float f)
{
        return C_vec2(a.x / f, a.y / f);
}

static inline c_vec3_t C_vec3_addf(c_vec3_t a, float f)
{
        return C_vec3(a.x + f, a.y + f, a.z + f);
}

static inline c_vec3_t C_vec3_subf(c_vec3_t a, float f)
{
        return C_vec3(a.x - f, a.y - f, a.z - f);
}

static inline c_vec3_t C_vec3_scalef(c_vec3_t a, float f)
{
        return C_vec3(a.x * f, a.y * f, a.z * f);
}

static inline c_vec3_t C_vec3_divf(c_vec3_t a, float f)
{
        return C_vec3(a.x / f, a.y / f, a.z / f);
}

static inline c_color_t C_color_addf(c_color_t a, float f)
{
        return C_color(a.r + f, a.g + f, a.b + f, a.a + f);
}

static inline c_color_t C_color_subf(c_color_t a, float f)
{
        return C_color(a.r - f, a.g - f, a.b - f, a.a - f);
}

static inline c_color_t C_color_scalef(c_color_t a, float f)
{
        return C_color(a.r * f, a.g * f, a.b * f, a.a * f);
}

static inline c_color_t C_color_divf(c_color_t a, float f)
{
        return C_color(a.r / f, a.g / f, a.b / f, a.a / f);
}

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
 Vector dimensions squared and summed. That is, magnitude without the square
 root operation.
\******************************************************************************/
static inline float C_vec2_square(c_vec2_t p)
{
    return p.x * p.x + p.y * p.y;
}

static inline float C_vec3_square(c_vec3_t p)
{
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

/******************************************************************************\
 Vector magnitude/length.
\******************************************************************************/
static inline float C_vec2_len(c_vec2_t p)
{
    return (float)sqrt(C_vec2_square(p));
}

static inline float C_vec3_len(c_vec3_t p)
{
    return (float)sqrt(C_vec3_square(p));
}

/******************************************************************************\
 Vector normalization.
\******************************************************************************/
static inline c_vec2_t C_vec2_norm(c_vec2_t p)
{
    return C_vec2_divf(p, C_vec2_len(p));
}

static inline c_vec3_t C_vec3_norm(c_vec3_t p)
{
    return C_vec3_divf(p, C_vec3_len(p));
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
 Truncate vector values down to whole numbers. Negative values are also
 truncated down (-2.1 is rounded to -3).
\******************************************************************************/
static inline float C_clamp(float value, float unit)
{
        if (value < 0)
                return (int)(value * -unit) / -unit;
        return (int)(value * unit) / unit;
}

static inline c_vec2_t C_vec2_clamp(c_vec2_t v, float u)
{
        return C_vec2(C_clamp(v.x, u), C_clamp(v.y, u));
}

static inline c_vec3_t C_vec3_clamp(c_vec3_t v, float u)
{
        return C_vec3(C_clamp(v.x, u), C_clamp(v.y, u), C_clamp(v.z, u));
}

/******************************************************************************\
 Returns TRUE if the two rectangles intersect.
\******************************************************************************/
static inline int C_rect_intersect(c_vec2_t o1, c_vec2_t s1,
                                   c_vec2_t o2, c_vec2_t s2)
{
        return o1.x <= o2.x + s2.x && o1.y <= o2.y + s2.y &&
               o1.x + s1.x >= o2.x && o1.y + s2.y >= o2.y;
}

/******************************************************************************\
 Limit a color to valid range.
\******************************************************************************/
static inline c_color_t C_color_limit(c_color_t c)
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
static inline c_color_t C_color_rgba(unsigned char r, unsigned char g,
                                     unsigned char b, unsigned char a)
{
    c_color_t result = { r, g, b, a };
    return C_color_divf(result, 255.f);
}

/******************************************************************************\
 Construct a color from a 32-bit ARGB integer.
\******************************************************************************/
static inline c_color_t C_color_32(unsigned int v)
{
    return C_color_rgba((v & 0xff0000) >> 16, (v & 0xff00) >> 8, v & 0xff,
                        (v & 0xff000000) >> 24);
}

