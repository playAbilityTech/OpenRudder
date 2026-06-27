#pragma once

#include "motion_sensor.h"

#define SENSOR_INPUTS_AVAILABLE IMU_AVAILABLE

bool sensor_inputs_init();
void sensor_inputs_recalibrate_orientation();
void sensor_inputs_recalibrate_sensors();
