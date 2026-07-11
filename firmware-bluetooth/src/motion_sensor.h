#pragma once

#include <stdbool.h>

#include <zephyr/devicetree.h>

#define IMU_HAS_LSM6DS3TR_C DT_NODE_EXISTS(DT_NODELABEL(lsm6ds3tr_c))
#define IMU_HAS_LSM9DS1_LOCAL (DT_NODE_EXISTS(DT_NODELABEL(lsm9ds1)) && DT_NODE_EXISTS(DT_NODELABEL(lsm9ds1_mag)))
#define IMU_HAS_MAGNETOMETER IMU_HAS_LSM9DS1_LOCAL
#define IMU_AVAILABLE (IMU_HAS_LSM6DS3TR_C || IMU_HAS_LSM9DS1_LOCAL)

bool motion_sensor_init(bool* has_magnetometer);
bool motion_sensor_read_imu(float* ax, float* ay, float* az,
                            float* gx, float* gy, float* gz);
bool motion_sensor_read_mag(float* mx, float* my, float* mz);
