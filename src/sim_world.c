#include "sim_world.h"

#include "edgeai_util.h"

void sim_world_init(sim_world_t *w, int32_t lcd_w, int32_t lcd_h)
{
    if (!w) return;
    w->ball.x_q16 = (lcd_w / 2) << 16;
    w->ball.y_q16 = (lcd_h / 2) << 16;
    w->ball.vx_q16 = 0;
    w->ball.vy_q16 = 0;
    w->ball.glint = 0;
}

void sim_step(sim_world_t *w, const sim_input_t *in, const sim_params_t *p)
{
    if (!w || !in || !p) return;

    /* soft_q15 * a_px_s2 gives Q15 px/s^2; convert to Q16 by <<1 */
    int32_t ax_a_q16 = (int32_t)(((int64_t)in->ax_soft_q15 * p->a_px_s2) << 1);
    int32_t ay_a_q16 = (int32_t)(((int64_t)in->ay_soft_q15 * p->a_px_s2) << 1);

    w->ball.vx_q16 += (int32_t)(((int64_t)ax_a_q16 * p->sim_step_q16) >> 16);
    w->ball.vy_q16 += (int32_t)(((int64_t)ay_a_q16 * p->sim_step_q16) >> 16);

    w->ball.vx_q16 = (int32_t)(((int64_t)w->ball.vx_q16 * p->damp_q16) >> 16);
    w->ball.vy_q16 = (int32_t)(((int64_t)w->ball.vy_q16 * p->damp_q16) >> 16);

    w->ball.x_q16 += (int32_t)(((int64_t)w->ball.vx_q16 * p->sim_step_q16) >> 16);
    w->ball.y_q16 += (int32_t)(((int64_t)w->ball.vy_q16 * p->sim_step_q16) >> 16);

    int32_t cx = w->ball.x_q16 >> 16;
    int32_t cy = w->ball.y_q16 >> 16;
    if (cx < p->minx) { cx = p->minx; w->ball.x_q16 = cx << 16; w->ball.vx_q16 = -(w->ball.vx_q16 * 3) / 4; }
    if (cx > p->maxx) { cx = p->maxx; w->ball.x_q16 = cx << 16; w->ball.vx_q16 = -(w->ball.vx_q16 * 3) / 4; }
    if (cy < p->miny) { cy = p->miny; w->ball.y_q16 = cy << 16; w->ball.vy_q16 = -(w->ball.vy_q16 * 3) / 4; }
    if (cy > p->maxy) { cy = p->maxy; w->ball.y_q16 = cy << 16; w->ball.vy_q16 = -(w->ball.vy_q16 * 3) / 4; }
}

