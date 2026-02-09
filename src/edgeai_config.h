#pragma once

#include <stdint.h>

/* Display geometry. */
#define EDGEAI_LCD_W 480
#define EDGEAI_LCD_H 320

/* Ball radius tuning.
 * Depth cue: smaller toward the top ("far"), larger toward the bottom ("near").
 */
#define EDGEAI_BALL_R_MIN 12
#define EDGEAI_BALL_R_MAX 34

/* Accelerometer normalization.
 * FXLS8974 configuration yields roughly ~512 counts per 1g in the current mode.
 */
#define EDGEAI_ACCEL_MAP_DENOM 512

/* Render tile limits (single-blit path). */
#define EDGEAI_TILE_MAX_W 200
#define EDGEAI_TILE_MAX_H 200

