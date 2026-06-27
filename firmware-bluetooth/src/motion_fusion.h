#pragma once

#include <stdbool.h>

void motion_fusion_reset(void);
void motion_fusion_update_imu(float gx, float gy, float gz,
                              float ax, float ay, float az,
                              float dt, float beta);
void motion_fusion_update_marg(float gx, float gy, float gz,
                               float ax, float ay, float az,
                               float mx, float my, float mz,
                               float dt, float beta);
float motion_fusion_get_pitch(void);
float motion_fusion_get_roll(void);
float motion_fusion_get_yaw(void);
float motion_fusion_wrap_angle_180(float angle);

void motion_fusion_reset_mag_calibration(void);
void motion_fusion_update_mag_calibration(float mx, float my, float mz);
void motion_fusion_apply_mag_calibration(float* mx, float* my, float* mz);
bool motion_fusion_is_mag_usable(float mx, float my, float mz);
