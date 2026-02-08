# EdgeAI Sand Demo (FRDM-MCXN947)

This repo is a hardware sandbox for FRDM-MCXN947 + PAR-LCD-S035 + Accel 4 Click (FXLS8974CF over mikroBUS/I2C).

Current demo (known-good):
- Tilt-controlled "silver ball" with shadow and motion trails (480x320).
- NPU is integrated via TFLM + Neutron backend and modulates the ball's specular "glint".

Key folders:
- `sdk_example/`: MCUX SDK example wrapper (built via `west`)
- `src/`: app + display + accel + NPU wrapper
- `tools/`: bootstrap/build/flash scripts

## Quickstart (Ubuntu)
1. Bootstrap user-local tools (no sudo): `./tools/bootstrap_ubuntu_user.sh`
2. Create/update local MCUX west workspace: `./tools/setup_mcuxsdk_ws.sh`
3. Build: `./tools/build_frdmmcxn947.sh debug`
4. Flash (requires NXP LinkServer installed): `./tools/flash_frdmmcxn947.sh`

Serial output (optional):
- `timeout 10 cat /dev/ttyACM0`

## Tuning / Orientation
Accel axis mapping macros live in `src/edgeai_sand_demo.c`:
- `EDGEAI_ACCEL_SWAP_XY`
- `EDGEAI_ACCEL_INVERT_X`
- `EDGEAI_ACCEL_INVERT_Y`

Milestone notes:
- `docs/MILESTONE_2026-02-08_TILT_BALL_NPU.md`
