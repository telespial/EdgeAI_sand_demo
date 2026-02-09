#include "npu_api.h"

#include "edgeai_util.h"

bool edgeai_npu_stub_init(edgeai_npu_state_t *s)
{
    if (!s) return false;
    return true;
}

bool edgeai_npu_stub_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out)
{
    (void)s;
    if (!in || !out) return false;

    /* Procedural modulation based on speed. */
    int32_t sx = edgeai_abs_i32(in->vx_q16 >> 16);
    int32_t sy = edgeai_abs_i32(in->vy_q16 >> 16);
    int32_t sp = edgeai_clamp_i32(sx + sy, 0, 255);
    out->glint = (uint8_t)sp;
    return true;
}

