# ToDo.md — EdgeAI Sand Demo Improvements (Architecture + Bug Fixes)

This ToDo list captures recommended improvements to keep the codebase clean as it evolves from a “ball + tilt” demo into a scalable sandbox for sand/water/particles + NPU-driven visuals.

It also includes **specific bug fixes** discovered during testing, with **clues tied to the current code** (dirty-rect blit, Q16 physics, LP-filtered accel).

---

## 0) Goals (why these changes exist)

1) **Scale from 1 entity (ball) → many entities (sand/water/particles)**
2) Keep **simulation and rendering decoupled**
3) Make NPU usage **modular** (Neutron backend vs stub)
4) Improve onboarding: “any engineer can build + run + extend”
5) Fix current behavior bugs so the demo feels “polished”

---

# A) BUG FIXES / POLISH (do these first)

## A1) Ball bounce perimeter is “square” and ignores current ball size (HIGH)

### Symptom
Ball bounces at inconsistent distances from the screen edge when ball size changes (perspective sizing). Looks like a square/canned boundary.

### Root cause clue (current code)
Physics bounds use constants based on **max radius** (e.g. `BALL_R_MAX`), but the ball’s draw radius is dynamic via something like `edgeai_ball_r_for_y(cy)`.

### Fix approach (minimal, matches current style)
Compute the **collision radius per sub-step** from the current `y` and use it for min/max bounds:

- Replace constant bounds (based on `BALL_R_MAX`) with:
  - `r_phys = edgeai_ball_r_for_y(cy_s)` (or equivalent)
  - `minx = r_phys + margin`, `maxx = LCD_W - 1 - (r_phys + margin)`
  - same for y

### Acceptance criteria
- As the ball grows/shrinks, the bounce distance to edge always matches the visible ball radius.
- No clipping at edges for “near” ball.
- No “floating off the wall” for “far” ball.

---

## A2) Trail dots sometimes get stuck on screen until the ball redraws (HIGH)

### Symptom
Occasional trail pixels remain permanently until the ball happens to pass over them again.

### Root cause clue (current code)
A **dirty rect** is used around ball + trails and a tile buffer is blitted. The tile size is clamped to `TILE_MAX_W/H`. If the dirty rect is larger than the tile, some regions (including removed trail points) are not redrawn → stale pixels remain on LCD.

### Fix approach (robust, minimal)
When the main dirty rect is clamped, perform a **small “erase blit”** around the *removed* trail point if that point lies outside the final blit rect:

- Track `removed_tx/removed_ty` (already present).
- After computing final `x0,y0,x1,y1` (post-clamp), if removed point is outside:
  - render background in a tiny rect centered on removed point
  - redraw any trails overlapping that rect
  - redraw ball if overlapping
  - blit that tiny rect

This preserves the performance advantages of small tiles while preventing stuck pixels.

### Acceptance criteria
- No stuck dots after several minutes of motion.
- Stuck dots do not appear even with fast movement / wide trail spread.
- Tile clamp can remain in place.

---

## A3) “Bang on table” should make the ball wobble/bounce; impacts are filtered out (HIGH)

### Symptom
Fast impulses (taps/bumps) barely move the ball. Tilt works, impacts do not.

### Root cause clues (current code)
- You use LP filtering (`ax_lp += (ax - ax_lp) >> n`), which removes high-frequency impulses.
- Deadzone logic further zeros small excursions.
- Impact energy often shows in **jerk** (delta / high-pass) and/or **Z** axis; both are suppressed if only the LP tilt term is fed into physics.

### Fix approach (add an “impact/jerk injection path” while keeping smooth tilt)
Keep the LP tilt path as-is, but add a second path:
- compute high-pass components: `ax_hp = ax - ax_lp`, `ay_hp = ay - ay_lp`, `az_hp = az - az_lp`
- compute `bang_score = abs(az_hp) + 0.5*abs(ax_hp) + 0.5*abs(ay_hp)`
- if `bang_score > threshold`, inject a short velocity impulse:
  - `vx_q16 += hx * bang_gain_q16`
  - `vy_q16 += hy * bang_gain_q16`
  - with `hx,hy` clamped to avoid runaway

Optionally reduce deadzone slightly (e.g. 10 → 6), but prefer the jerk path so tilt remains stable.

### Acceptance criteria
- Normal tilting remains smooth.
- A sharp tap causes a visible wobble/kick.
- Strong taps produce stronger kick without instability.
- Works consistently across sensor mounting orientations (after axis mapping).

---

## A4) After title, screen goes black; dune background only appears when “painted” by ball movement (CRITICAL)

### Symptom
After the title is shown, the screen becomes black. The background reappears only where the ball/trail dirty-rect redraws, so it looks like the ball “paints” the dunes onto the screen.

### Root cause clue (current code)
The renderer uses incremental dirty-rect updates. After the title phase ends, the LCD is not refreshed with the dune background, and only future dirty-rect blits update small regions.

### Fix approach (correct for tile-based renderer)
Add a **one-time full-screen background fill** (tiled) when transitioning from title → gameplay:

- Add a flag: `need_full_bg_redraw = true` when title ends.
- On next normal frame:
  - call `draw_full_background(tile)` that loops over the screen in `TILE_MAX_W/H` chunks:
    - `sw_render_dune_bg(tile, w, h, x0, y0)`
    - `par_lcd_s035_blit_rect(x0,y0,x1,y1,tile)`
  - set `need_full_bg_redraw = false`

Optional: clear/reset trail state on transition to prevent “ghost trails” from title phase.

### Acceptance criteria
- After title, dunes appear immediately across full screen.
- No “painted-in” look.
- Works even if the ball never moves.

---

# B) ARCHITECTURE IMPROVEMENTS (do after bugs are fixed)

## B1) Simulation: Make boundaries explicit (HIGH)

### B1.1 Introduce simulation input + world structs
**Why**
Prevents sim/render mixing and enables scaling to sand/water.

**Task**
Create:

```c
typedef struct {
    float dt;
    accel_state_t accel;  // post-mapped + post-filtered
    int32_t bang_score;   // optional, if bang is computed in the input stage
    int32_t ax_hp, ay_hp; // optional for impulse injection
} sim_input_t;

typedef struct {
    ball_state_t ball;
    // future: sand_state_t sand; water_state_t water;
} sim_world_t;

Acceptance criteria

    Main loop builds sim_input_t

    sim_step(&world, &input) owns all physics updates

    Render takes only sim_world_t

B1.2 Create sim_step() as the only sim entry point

Task
Add:

    sim_world_init(sim_world_t*)

    sim_step(sim_world_t*, const sim_input_t*)

Acceptance criteria

    No sim rules live outside sim_*.c

    Main loop is orchestration only

B1.3 Add src/sim_world.h now (even if small)

Why
Prevents state scattering and demo-loop entropy.

Acceptance criteria

    One canonical world struct used everywhere

B2) Rendering: Prepare for sand/water complexity (HIGH)
B2.1 Split “render primitives” vs “render world”

Task

    Keep primitives (circles, blits, etc.) in sw_render.c

    Add render_world.c that draws dunes, ball, trails, overlays from sim_world_t

Acceptance criteria

    Simulation never calls primitive draw routines

    World drawing logic is centralized and composable

B2.2 LCD driver clarity note (MED)

Add a note in the LCD driver header:

    This is intentionally a simple single-buffer blit approach

    Future DMA/double-buffer/partial redraw lives here

B3) Accelerometer: Centralize axis map + filtering rationale (MED)
B3.1 Centralize axis mapping

Task
Move SWAP_XY/INVERT logic into one function:

    accel_apply_axis_map(raw, &mapped)

Acceptance criteria

    Axis mapping is defined in exactly one place

    Physics/render never needs to care about sensor orientation

B3.2 Document filter intent

Add a block comment near filter constants describing:

    why the filter exists

    what it’s tuned for (tilt stability vs responsiveness)

    what symptoms indicate it’s wrong

B4) NPU/TFLM: Make it swappable + testable (HIGH)
B4.1 Create backends: neutron vs stub

Task

    npu_api.h

    npu_backend_neutron.c

    npu_backend_stub.c

Acceptance criteria

    Build can run without Neutron

    Easy performance comparison (NPU on/off)

B4.2 Keep NPU used for perceptual modulation

Document and enforce:

    NPU outputs modulate appearance (glint, texture, shading)

    Physics remains deterministic and debuggable

C) TOOLING / DOCS / QUALITY (MED)
C1) README: tested versions + validation checklist

Add:

    Ubuntu version

    West version

    toolchain version

    LinkServer version

    “Expected output” (LCD + serial)

---

# B) Findings From NPU/Model Bring-Up Debug (2026-02-09)

## B1) Symptom: splash/title shown, then black screen "freeze"

Observed pattern: boot title or smoke-test banner is rendered, then the display remains black and the program no longer progresses.

High-probability cause: stack overflow or memory corruption during the first model inference call (`Invoke`), rather than a renderer/LCD issue.

## B2) Evidence From Debug Attach

When attaching a debugger after the freeze, register state can show:
- `pc` and `sp` inside the tensor arena region (RAM buffer), with an invalid `lr`
- corrupted backtrace

This pattern is consistent with stack overflow corrupting control flow, or a write beyond bounds into stack/arena memory.

## B3) NPU Status Reading Stays 0

If NPU counters are incremented on successful inference completion, a hard fault/hang inside the first `Invoke` will keep NPU counters at 0 even if model initialization succeeds.

Additionally, the baseline ball + dune renderer does not execute any NPU workload, so NPU counters remain 0 in the demo path by design.

## B4) Recommended Next Step (Bring-Up Order)

1. Increase stack size (main stack and ISR stack) substantially for the model smoke-test build.
2. Re-run single-inference smoke test and confirm progression past the pre-invoke stage marker.
3. If inference completes, re-enable NPU status overlay and confirm non-zero inference counters.

C2) Docs: add docs/START_HERE.md

Provide reading order:

    HARDWARE

    BUILD/FLASH

    ACCEL BRINGUP

    SIM ARCH

    NPU PLAN

C3) Basic CI build

Add GitHub Action:

    west build for a known config

    optional format/lint if desired

C4) Timing instrumentation

Measure per frame:

    input read/filter

    sim step

    render

    LCD blit
    Expose FPS or ms-stage logs.

Suggested implementation order (fastest payoff)

    A4 Full background redraw after title

    A1 Bounce bounds follow dynamic radius

    A2 Trail stuck pixel fix (erase blit on clamp)

    A3 Bang impulse path (high-pass/jerk injection)

    B1 sim boundary (world/input/step)

    B2 render boundary (world vs primitives)

    C4 timing instrumentation + baseline

    B4 NPU backend split

    C1/C2 docs polish + C3 CI
