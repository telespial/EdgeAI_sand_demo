#pragma once

#include <stdint.h>

static inline int32_t edgeai_abs_i32(int32_t v) { return (v < 0) ? -v : v; }

static inline int32_t edgeai_clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline int32_t edgeai_clamp_i32_sym(int32_t v, int32_t limit_abs)
{
    if (v > limit_abs) return limit_abs;
    if (v < -limit_abs) return -limit_abs;
    return v;
}

static inline void edgeai_u32_to_dec3(char out[4], uint32_t v)
{
    if (v > 999u) v = 999u;
    out[0] = (char)('0' + (v / 100u));
    out[1] = (char)('0' + ((v / 10u) % 10u));
    out[2] = (char)('0' + (v % 10u));
    out[3] = '\0';
}

