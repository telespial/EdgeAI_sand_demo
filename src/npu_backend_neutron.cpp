#include "npu_api.h"

#include <stdint.h>

#include "edgeai_util.h"
#include "npu/model.h"

/* Neutron backend: wraps the existing TFLM+Neutron integration and reduces output to a single 0..255 glint value. */

static bool s_inited = false;
static edgeai_tensor_dims_t s_in_dims = {0};
static edgeai_tensor_type_t s_in_type = kEdgeAiTensorType_UINT8;
static uint8_t *s_in_data = nullptr;
static edgeai_tensor_dims_t s_out_dims = {0};
static edgeai_tensor_type_t s_out_type = kEdgeAiTensorType_UINT8;
static uint8_t *s_out_data = nullptr;

static uint32_t edgeai_dims_elems(const edgeai_tensor_dims_t *d)
{
    if (!d) return 0;
    uint32_t n = 1;
    for (uint32_t i = 0; i < (uint32_t)d->size; i++)
    {
        if (d->data[i] == 0) return 0;
        n *= (uint32_t)d->data[i];
    }
    return n;
}

extern "C" bool edgeai_npu_neutron_init(edgeai_npu_state_t *s)
{
    if (!s) return false;
    s_inited = false;
    s_in_data = nullptr;
    s_out_data = nullptr;
    s_in_dims = {0};
    s_out_dims = {0};
    s_in_type = kEdgeAiTensorType_UINT8;
    s_out_type = kEdgeAiTensorType_UINT8;

    /* Model init is known to fault on some setups; keep it gated. */
    if (!EDGEAI_ENABLE_NPU_INFERENCE) return false;

    bool ok = (EDGEAI_MODEL_Init() == kStatus_Success);
    if (!ok) return false;

    s_in_data = EDGEAI_MODEL_GetInputTensorData(&s_in_dims, &s_in_type);
    s_out_data = EDGEAI_MODEL_GetOutputTensorData(&s_out_dims, &s_out_type);
    ok = (s_in_data != nullptr) && (s_out_data != nullptr);
    s_inited = ok;
    return ok;
}

extern "C" bool edgeai_npu_neutron_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out)
{
    (void)s;
    if (!s_inited || !in || !out) return false;

    uint32_t n = edgeai_dims_elems(&s_in_dims);
    if (n == 0) n = 1;

    uint8_t base = 127u;
    uint8_t amp = (uint8_t)edgeai_clamp_i32(edgeai_abs_i32(in->vx_q16 >> 16) + edgeai_abs_i32(in->vy_q16 >> 16), 0, 80);
    for (uint32_t i = 0; i < n; i++)
    {
        uint8_t t = (uint8_t)(i & 31u);
        uint8_t v = (t < 16u) ? (uint8_t)(base + (amp * t) / 16u) : (uint8_t)(base + (amp * (31u - t)) / 16u);
        s_in_data[i] = v;
    }

    if (EDGEAI_MODEL_RunInference() != kStatus_Success) return false;

    uint32_t out_n = edgeai_dims_elems(&s_out_dims);
    if (out_n == 0) out_n = 1;

    if (s_out_type == kEdgeAiTensorType_UINT8)
    {
        uint32_t m = 0;
        for (uint32_t i = 0; i < out_n; i++) if (s_out_data[i] > m) m = s_out_data[i];
        out->glint = (uint8_t)m;
        return true;
    }
    if (s_out_type == kEdgeAiTensorType_INT8)
    {
        int8_t *p = (int8_t *)s_out_data;
        int32_t mm = -128;
        for (uint32_t i = 0; i < out_n; i++) if ((int32_t)p[i] > mm) mm = (int32_t)p[i];
        out->glint = (uint8_t)edgeai_clamp_i32(mm + 128, 0, 255);
        return true;
    }

    float *p = (float *)s_out_data;
    float mm = p[0];
    for (uint32_t i = 1; i < out_n; i++) if (p[i] > mm) mm = p[i];
    if (mm < 0.0f) mm = 0.0f;
    if (mm > 1.0f) mm = 1.0f;
    out->glint = (uint8_t)(mm * 255.0f);
    return true;
}
