/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EDGEAI_NPU_MODEL_H_
#define _EDGEAI_NPU_MODEL_H_

#include <stdint.h>

#include "fsl_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define EDGEAI_MAX_TENSOR_DIMS 4

typedef struct
{
    uint32_t size;
    uint32_t data[EDGEAI_MAX_TENSOR_DIMS];
} edgeai_tensor_dims_t;

typedef enum
{
    kEdgeAiTensorType_FLOAT32 = 0,
    kEdgeAiTensorType_UINT8   = 1,
    kEdgeAiTensorType_INT8    = 2
} edgeai_tensor_type_t;

status_t EDGEAI_MODEL_Init(void);
uint8_t *EDGEAI_MODEL_GetInputTensorData(edgeai_tensor_dims_t *dims, edgeai_tensor_type_t *type);
uint8_t *EDGEAI_MODEL_GetOutputTensorData(edgeai_tensor_dims_t *dims, edgeai_tensor_type_t *type);
status_t EDGEAI_MODEL_RunInference(void);

#if defined(__cplusplus)
}
#endif

#endif /* _EDGEAI_NPU_MODEL_H_ */

