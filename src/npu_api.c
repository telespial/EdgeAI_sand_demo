#include "npu_api.h"

bool edgeai_npu_stub_init(edgeai_npu_state_t *s);
bool edgeai_npu_stub_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out);

bool edgeai_npu_neutron_init(edgeai_npu_state_t *s);
bool edgeai_npu_neutron_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out);

edgeai_npu_backend_t edgeai_npu_backend(void)
{
    if (EDGEAI_NPU_BACKEND == (int)kEdgeAiNpuBackendNeutron) return kEdgeAiNpuBackendNeutron;
    return kEdgeAiNpuBackendStub;
}

char edgeai_npu_backend_char(void)
{
    return (edgeai_npu_backend() == kEdgeAiNpuBackendNeutron) ? 'N' : 'S';
}

bool edgeai_npu_init(edgeai_npu_state_t *s)
{
    if (!s) return false;
    s->init_ok = false;

    if (edgeai_npu_backend() == kEdgeAiNpuBackendNeutron)
    {
        s->init_ok = edgeai_npu_neutron_init(s);
        return s->init_ok;
    }

    s->init_ok = edgeai_npu_stub_init(s);
    return s->init_ok;
}

bool edgeai_npu_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out)
{
    if (!s || !in || !out) return false;
    if (!EDGEAI_ENABLE_NPU_INFERENCE) return false;
    if (!s->init_ok) return false;

    if (edgeai_npu_backend() == kEdgeAiNpuBackendNeutron)
    {
        return edgeai_npu_neutron_step(s, in, out);
    }
    return edgeai_npu_stub_step(s, in, out);
}

