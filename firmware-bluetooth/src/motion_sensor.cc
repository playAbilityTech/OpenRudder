#include "motion_sensor.h"

#include "lsm9ds1_local.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#if IMU_AVAILABLE

#define IMU_ODR_FREQUENCY 52
#define ACCEL_SCALE_RANGE 2
#define GYRO_SCALE_RANGE 125

#if IMU_HAS_LSM6DS3TR_C
static const struct device* imu_dev;
#endif

bool motion_sensor_init(bool* has_magnetometer) {
#if IMU_HAS_LSM9DS1_LOCAL
    if (!lsm9ds1_local_init()) {
        return false;
    }
    *has_magnetometer = true;
    return true;
#elif IMU_HAS_LSM6DS3TR_C
    imu_dev = DEVICE_DT_GET(DT_NODELABEL(lsm6ds3tr_c));

    if (!device_is_ready(imu_dev)) {
        return false;
    }

    struct sensor_value odr_attr;
    odr_attr.val1 = IMU_ODR_FREQUENCY;
    odr_attr.val2 = 0;

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        return false;
    }

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        return false;
    }

    struct sensor_value accel_scale_attr;
    accel_scale_attr.val1 = ACCEL_SCALE_RANGE;
    accel_scale_attr.val2 = 0;

    sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ,
                    SENSOR_ATTR_FULL_SCALE, &accel_scale_attr);

    struct sensor_value angular_scale_attr;
    angular_scale_attr.val1 = GYRO_SCALE_RANGE;
    angular_scale_attr.val2 = 0;

    sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ,
                    SENSOR_ATTR_FULL_SCALE, &angular_scale_attr);

    *has_magnetometer = false;
    return true;
#else
    ARG_UNUSED(has_magnetometer);
    return false;
#endif
}

bool motion_sensor_read_imu(float* ax, float* ay, float* az,
                            float* gx, float* gy, float* gz) {
#if IMU_HAS_LSM9DS1_LOCAL
    if (lsm9ds1_local_read_imu(ax, ay, az, gx, gy, gz)) {
        return true;
    }
#endif

#if IMU_HAS_LSM6DS3TR_C
    struct sensor_value accel[3], gyro[3];

    if (!imu_dev || sensor_sample_fetch(imu_dev) < 0) {
        return false;
    }

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &accel[0]) < 0 ||
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]) < 0 ||
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]) < 0) {
        return false;
    }

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_X, &gyro[0]) < 0 ||
        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Y, &gyro[1]) < 0 ||
        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_Z, &gyro[2]) < 0) {
        return false;
    }

    *ax = (float)sensor_value_to_double(&accel[0]);
    *ay = (float)sensor_value_to_double(&accel[1]);
    *az = (float)sensor_value_to_double(&accel[2]);
    *gx = (float)sensor_value_to_double(&gyro[0]);
    *gy = (float)sensor_value_to_double(&gyro[1]);
    *gz = (float)sensor_value_to_double(&gyro[2]);

    return true;
#else
    ARG_UNUSED(ax);
    ARG_UNUSED(ay);
    ARG_UNUSED(az);
    ARG_UNUSED(gx);
    ARG_UNUSED(gy);
    ARG_UNUSED(gz);
    return false;
#endif
}

bool motion_sensor_read_mag(float* mx, float* my, float* mz) {
#if IMU_HAS_LSM9DS1_LOCAL
    return lsm9ds1_local_read_mag(mx, my, mz);
#else
    ARG_UNUSED(mx);
    ARG_UNUSED(my);
    ARG_UNUSED(mz);
    return false;
#endif
}

#else

bool motion_sensor_init(bool* has_magnetometer) {
    ARG_UNUSED(has_magnetometer);
    return false;
}

bool motion_sensor_read_imu(float* ax, float* ay, float* az,
                            float* gx, float* gy, float* gz) {
    ARG_UNUSED(ax);
    ARG_UNUSED(ay);
    ARG_UNUSED(az);
    ARG_UNUSED(gx);
    ARG_UNUSED(gy);
    ARG_UNUSED(gz);
    return false;
}

bool motion_sensor_read_mag(float* mx, float* my, float* mz) {
    ARG_UNUSED(mx);
    ARG_UNUSED(my);
    ARG_UNUSED(mz);
    return false;
}

#endif
