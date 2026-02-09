#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    kEdgeAiNpuBackendStub = 0,
    kEdgeAiNpuBackendNeutron = 1,
} edgeai_npu_backend_t;

typedef struct
{
    bool init_ok;
} edgeai_npu_state_t;

typedef struct
{
    int32_t vx_q16;
    int32_t vy_q16;
} edgeai_npu_input_t;

typedef struct
{
    uint8_t glint;
} edgeai_npu_output_t;

/* NPU run gating.
 * Default is disabled; enable only after the selected backend is validated.
 */
#ifndef EDGEAI_ENABLE_NPU_INFERENCE
#define EDGEAI_ENABLE_NPU_INFERENCE 0
#endif

/* Backend selection: 0=stub, 1=neutron(TFLM+LinkServer). */
#ifndef EDGEAI_NPU_BACKEND
#define EDGEAI_NPU_BACKEND ((int)kEdgeAiNpuBackendStub)
#endif

edgeai_npu_backend_t edgeai_npu_backend(void);
char edgeai_npu_backend_char(void);

bool edgeai_npu_init(edgeai_npu_state_t *s);
bool edgeai_npu_step(edgeai_npu_state_t *s, const edgeai_npu_input_t *in, edgeai_npu_output_t *out);

