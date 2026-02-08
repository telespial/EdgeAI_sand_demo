/*
 * Minimal TensorFlow Lite Micro wrapper for running a Neutron NPU model.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "npu/model.h"

#include <assert.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "npu/model_data.h"

static const tflite::Model *s_model = nullptr;
static tflite::MicroInterpreter *s_interpreter = nullptr;

extern tflite::MicroOpResolver &EDGEAI_MODEL_GetOpsResolver();

/* Tensor arena for TFLM. Size comes from the model header (kTensorArenaSize). */
static uint8_t s_tensorArena[kTensorArenaSize] __attribute__((aligned(16)));

static uint8_t *get_tensor_data(TfLiteTensor *tensor, edgeai_tensor_dims_t *dims, edgeai_tensor_type_t *type)
{
    assert(tensor != nullptr);
    assert(dims != nullptr);
    assert(type != nullptr);

    switch (tensor->type)
    {
        case kTfLiteFloat32: *type = kEdgeAiTensorType_FLOAT32; break;
        case kTfLiteUInt8:   *type = kEdgeAiTensorType_UINT8; break;
        case kTfLiteInt8:    *type = kEdgeAiTensorType_INT8; break;
        default: assert(false && "Unsupported tensor type"); break;
    }

    dims->size = (uint32_t)tensor->dims->size;
    assert(dims->size <= EDGEAI_MAX_TENSOR_DIMS);
    for (int i = 0; i < tensor->dims->size; i++)
    {
        dims->data[i] = (uint32_t)tensor->dims->data[i];
    }

    return tensor->data.uint8;
}

status_t EDGEAI_MODEL_Init(void)
{
    s_model = tflite::GetModel(model_data);
    if (!s_model || (s_model->version() != TFLITE_SCHEMA_VERSION))
    {
        return kStatus_Fail;
    }

    tflite::MicroOpResolver &resolver = EDGEAI_MODEL_GetOpsResolver();

    static tflite::MicroInterpreter static_interpreter(s_model, resolver, s_tensorArena, kTensorArenaSize);
    s_interpreter = &static_interpreter;

    if (s_interpreter->AllocateTensors() != kTfLiteOk)
    {
        return kStatus_Fail;
    }

    return kStatus_Success;
}

uint8_t *EDGEAI_MODEL_GetInputTensorData(edgeai_tensor_dims_t *dims, edgeai_tensor_type_t *type)
{
    if (!s_interpreter) return nullptr;
    return get_tensor_data(s_interpreter->input(0), dims, type);
}

uint8_t *EDGEAI_MODEL_GetOutputTensorData(edgeai_tensor_dims_t *dims, edgeai_tensor_type_t *type)
{
    if (!s_interpreter) return nullptr;
    return get_tensor_data(s_interpreter->output(0), dims, type);
}

status_t EDGEAI_MODEL_RunInference(void)
{
    if (!s_interpreter) return kStatus_Fail;
    return (s_interpreter->Invoke() == kTfLiteOk) ? kStatus_Success : kStatus_Fail;
}

