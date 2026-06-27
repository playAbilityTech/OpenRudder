#include "sensor_inputs.h"
#include "activity_led.h"
#include "imu_descriptor.h"
#include "mic_level.h"
#include "motion_fusion.h"
#include "motion_sensor.h"
#include "config.h"
#include "descriptor_parser.h"
#include "globals.h"
#include "platform.h"
#include "remapper.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <math.h>

#if IMU_AVAILABLE

#define IMU_VIRTUAL_INTERFACE 0x1000
#define CALIBRATION_SAMPLES 200

#define IMU_SAMPLE_RATE_MS 15
#define MAX_ERROR_COUNT_BEFORE_BACKOFF 5
#define ERROR_BACKOFF_MULTIPLIER 4
#define CALIBRATION_RETRY_DELAY_MS 500

#define GRAVITY 9.81f

#define LED_ACTIVITY_DURATION_MS 50

#define MIN_DT_SECONDS 0.005f
#define MAX_DT_SECONDS 0.050f
#define EXPECTED_DT_SECONDS 0.015f
#define CALIBRATION_SAMPLE_DELAY_MS 5

#define MAX_FILTER_BUFFER_SIZE 16

typedef struct {
    float beta_base;
    float beta_min;
    float beta_max;
    float stationary_threshold;
    float accel_trust_threshold_high;
    float accel_trust_threshold_low;
    float bias_update_rate;
    float gyro_deadzone;
    float angle_clamp_limit;
    float magnitude_filter_alpha;
} imu_config_t;

static imu_config_t imu_config = {
    .beta_base = 0.1f,
    .beta_min = 0.01f,
    .beta_max = 0.3f,
    .stationary_threshold = 0.01f,
    .accel_trust_threshold_high = 2.0f,
    .accel_trust_threshold_low = 0.5f,
    .bias_update_rate = 0.001f,
    .gyro_deadzone = 0.001f,
    .angle_clamp_limit = 45.0f,
    .magnitude_filter_alpha = 0.9f
};

typedef struct {
    float y, alpha;
} iir_t;

typedef struct {
    float buffer[MAX_FILTER_BUFFER_SIZE];
    int index;
    int count;
    int size;
    bool initialized;
} moving_avg_filter_t;

static void imu_work_fn(struct k_work* work);
static K_WORK_DELAYABLE_DEFINE(imu_work, imu_work_fn);

static bool has_magnetometer = false;
static volatile float pitch_offset = 0.0f;
static volatile float roll_offset = 0.0f;
static volatile float yaw_offset = 0.0f;
static bool orientation_offset_initialized = false;
static bool yaw_reference_initialized = false;
static int64_t last_timestamp = 0;

static volatile float gyro_bias_x = 0.0f;
static volatile float gyro_bias_y = 0.0f;
static volatile float gyro_bias_z = 0.0f;
static float accel_bias_x = 0.0f;
static float accel_bias_y = 0.0f;
static float accel_bias_z = 0.0f;
static bool is_calibrated = false;

static iir_t magnitude_filter = {.y = 9.81f, .alpha = 0.9f};
static uint32_t error_count = 0;

static uint8_t last_known_angle_clamp_limit = 90;

static moving_avg_filter_t pitch_filter = {
    .index = 0,
    .count = 0,
    .initialized = false
};

static moving_avg_filter_t roll_filter = {
    .index = 0,
    .count = 0,
    .initialized = false
};

static moving_avg_filter_t yaw_filter = {
    .index = 0,
    .count = 0,
    .initialized = false
};

static float compute_dynamic_beta(float hp_magnitude) {
    if (hp_magnitude < imu_config.accel_trust_threshold_low) {
        return imu_config.beta_max;
    } else if (hp_magnitude > imu_config.accel_trust_threshold_high) {
        return imu_config.beta_min;
    } else {
        float ratio = (hp_magnitude - imu_config.accel_trust_threshold_low) / 
                     (imu_config.accel_trust_threshold_high - imu_config.accel_trust_threshold_low);
        return imu_config.beta_max - ratio * (imu_config.beta_max - imu_config.beta_min);
    }
}

static void reset_orientation_filters(void) {
    int adaptive_buffer_size = imu_filter_buffer_size;

    pitch_filter.size = adaptive_buffer_size;
    roll_filter.size = adaptive_buffer_size;
    yaw_filter.size = adaptive_buffer_size;

    pitch_filter.initialized = false;
    roll_filter.initialized = false;
    yaw_filter.initialized = false;

    pitch_filter.count = 0;
    roll_filter.count = 0;
    yaw_filter.count = 0;

    pitch_filter.index = 0;
    roll_filter.index = 0;
    yaw_filter.index = 0;
}

static int16_t scale_angle_to_int16(float angle, float min_angle, float max_angle) {
    angle = fmaxf(min_angle, fminf(max_angle, angle));
    float normalized = (angle - min_angle) / (max_angle - min_angle);

    int scaled = (int)((normalized - 0.5f) * 65535.0f);
    return (int16_t)fmaxf(-32768.0f, fminf(32767.0f, (float)scaled));
}

static uint16_t scale_magnitude_to_uint16(float magnitude, float max_magnitude) {
    magnitude = fmaxf(0.0f, fminf(max_magnitude, magnitude));
    float normalized = magnitude / max_magnitude;
    int scaled = (int)(normalized * 255.0f);
    return (uint16_t)fmaxf(0.0f, fminf(255.0f, (float)scaled));
}

static void clamp_angle_to_limit(float* angle) {
    float current_clamp_limit = (float)imu_angle_clamp_limit;
    *angle = fmaxf(-current_clamp_limit, fminf(current_clamp_limit, *angle));
}

static float apply_deadzone(float value, float deadzone) {
    if (fabsf(value) < deadzone) {
        return 0.0f;
    }
    return value > 0 ? value - deadzone : value + deadzone;
}

static void calibrate_orientation(float pitch, float roll, float yaw) {
    pitch_offset = pitch;
    roll_offset = roll;
    yaw_offset = yaw;
    orientation_offset_initialized = true;
}

static bool calibrate_sensors(void) {
    float sum_accel_x = 0.0f, sum_accel_y = 0.0f, sum_accel_z = 0.0f;
    float sum_gyro_x = 0.0f, sum_gyro_y = 0.0f, sum_gyro_z = 0.0f;
    
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        float ax, ay, az, gx, gy, gz;
        if (!motion_sensor_read_imu(&ax, &ay, &az, &gx, &gy, &gz)) {
            return false;
        }
        
        sum_accel_x += ax;
        sum_accel_y += ay;
        sum_accel_z += az;
        sum_gyro_x += gx;
        sum_gyro_y += gy;
        sum_gyro_z += gz;

        k_msleep(CALIBRATION_SAMPLE_DELAY_MS);
    }
    
    accel_bias_x = sum_accel_x / CALIBRATION_SAMPLES;
    accel_bias_y = sum_accel_y / CALIBRATION_SAMPLES;
    accel_bias_z = sum_accel_z / CALIBRATION_SAMPLES - GRAVITY;
    
    gyro_bias_x = sum_gyro_x / CALIBRATION_SAMPLES;
    gyro_bias_y = sum_gyro_y / CALIBRATION_SAMPLES;
    gyro_bias_z = sum_gyro_z / CALIBRATION_SAMPLES;
    
    return true;
}

static float iir_update_magnitude(iir_t *filter, float input) {
    filter->y = filter->alpha * filter->y + (1.0f - filter->alpha) * input;
    return filter->y;
}

static float moving_avg_filter_update(moving_avg_filter_t *filter, float input) {
    int bufsize = filter->size;
    if (bufsize < 1) bufsize = 1;
    if (bufsize > MAX_FILTER_BUFFER_SIZE) bufsize = MAX_FILTER_BUFFER_SIZE;
    filter->buffer[filter->index] = input;
    filter->index = (filter->index + 1) % bufsize;
    
    if (!filter->initialized) {
        filter->initialized = true;
        filter->count = 1;
        return input;
    }
    
    if (filter->count < bufsize) {
        filter->count++;
    }
    
    float sum = 0.0f;
    for (int i = 0; i < filter->count; i++) {
        sum += filter->buffer[i];
    }
    
    return sum / filter->count;
}

static void update_gyro_bias_if_stationary(float gx_raw, float gy_raw, float gz_raw, float hp_magnitude) {
    if (hp_magnitude < imu_config.stationary_threshold) {
        gyro_bias_x += imu_config.bias_update_rate * (gx_raw - gyro_bias_x);
        gyro_bias_y += imu_config.bias_update_rate * (gy_raw - gyro_bias_y);
        gyro_bias_z += imu_config.bias_update_rate * (gz_raw - gyro_bias_z);
    }
}

static void imu_work_fn(struct k_work* work) {
    int64_t now = k_uptime_get();
    float dt = last_timestamp ? (now - last_timestamp) / 1000.0f : EXPECTED_DT_SECONDS;
    last_timestamp = now;


    if (dt < MIN_DT_SECONDS || dt > MAX_DT_SECONDS) {
        dt = EXPECTED_DT_SECONDS;
    }

    if (!is_calibrated) {
        if (calibrate_sensors()) {
            is_calibrated = true;
            magnitude_filter.alpha = imu_config.magnitude_filter_alpha;
            orientation_offset_initialized = false;
            mic_level_init();
        } else {
            error_count++;

            k_work_reschedule(&imu_work, K_MSEC(CALIBRATION_RETRY_DELAY_MS));
            return;
        }
    }

    float ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
    if (!motion_sensor_read_imu(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw)) {
        error_count++;
        activity_led_off();
        

        uint32_t delay_ms = (error_count > MAX_ERROR_COUNT_BEFORE_BACKOFF) ? 
                           IMU_SAMPLE_RATE_MS * ERROR_BACKOFF_MULTIPLIER : 
                           IMU_SAMPLE_RATE_MS;
        k_work_reschedule(&imu_work, K_MSEC(delay_ms));
        return;
    }

    float mx_raw = 0.0f, my_raw = 0.0f, mz_raw = 0.0f;
    float mx = 0.0f, my = 0.0f, mz = 0.0f;
    bool mag_usable = false;
    if (has_magnetometer && !motion_sensor_read_mag(&mx_raw, &my_raw, &mz_raw)) {
        error_count++;
        activity_led_off();

        uint32_t delay_ms = (error_count > MAX_ERROR_COUNT_BEFORE_BACKOFF) ?
                           IMU_SAMPLE_RATE_MS * ERROR_BACKOFF_MULTIPLIER :
                           IMU_SAMPLE_RATE_MS;
        k_work_reschedule(&imu_work, K_MSEC(delay_ms));
        return;
    }

    if (has_magnetometer) {
        motion_fusion_update_mag_calibration(mx_raw, my_raw, mz_raw);
        mx = mx_raw;
        my = my_raw;
        mz = mz_raw;
        motion_fusion_apply_mag_calibration(&mx, &my, &mz);
        mag_usable = motion_fusion_is_mag_usable(mx, my, mz);
    }
    
    error_count = 0;
    
    if (last_known_angle_clamp_limit != imu_angle_clamp_limit) {
        last_known_angle_clamp_limit = imu_angle_clamp_limit;
        imu_config.angle_clamp_limit = (float)imu_angle_clamp_limit;
        reset_orientation_filters();
    }
    
    float ax = ax_raw - accel_bias_x;
    float ay = ay_raw - accel_bias_y;
    float az = az_raw - accel_bias_z;
    float gx = gx_raw - gyro_bias_x;
    float gy = gy_raw - gyro_bias_y;
    float gz = gz_raw - gyro_bias_z;
    
    gx = apply_deadzone(gx, imu_config.gyro_deadzone);
    gy = apply_deadzone(gy, imu_config.gyro_deadzone);
    gz = apply_deadzone(gz, imu_config.gyro_deadzone);
    
    float accel_mag = sqrtf(ax * ax + ay * ay + az * az);
    float filtered_mag = iir_update_magnitude(&magnitude_filter, accel_mag);
    float hp_magnitude = accel_mag - filtered_mag;
    
    update_gyro_bias_if_stationary(gx_raw, gy_raw, gz_raw, fabsf(hp_magnitude));
    
    float beta = compute_dynamic_beta(fabsf(hp_magnitude));
    
    if (mag_usable) {
        motion_fusion_update_marg(gx, gy, gz, ax, ay, az, mx, my, mz, dt, beta);
        yaw_reference_initialized = true;
    } else {
        motion_fusion_update_imu(gx, gy, gz, ax, ay, az, dt, beta);
    }
    
    float pitch = motion_fusion_get_pitch();
    float roll = motion_fusion_get_roll();
    bool yaw_valid = has_magnetometer && yaw_reference_initialized;
    float yaw = yaw_valid ? motion_fusion_get_yaw() : 0.0f;

    if (!orientation_offset_initialized && (!has_magnetometer || yaw_valid)) {
        calibrate_orientation(pitch, roll, yaw_valid ? yaw : 0.0f);
    }

    if (!orientation_offset_initialized) {
        k_work_reschedule(&imu_work, K_MSEC(IMU_SAMPLE_RATE_MS));
        return;
    }
    
    float pitch_corrected = -(pitch - pitch_offset);
    float roll_corrected = roll - roll_offset;
    float yaw_corrected = yaw_valid ? motion_fusion_wrap_angle_180(yaw - yaw_offset) : 0.0f;
    
    if (imu_pitch_inverted) {
        pitch_corrected = -pitch_corrected;
    }
    if (imu_roll_inverted) {
        roll_corrected = -roll_corrected;
    }
    if (imu_yaw_inverted) {
        yaw_corrected = -yaw_corrected;
    }
    
    pitch_corrected = moving_avg_filter_update(&pitch_filter, pitch_corrected);
    roll_corrected = moving_avg_filter_update(&roll_filter, roll_corrected);
    yaw_corrected = moving_avg_filter_update(&yaw_filter, yaw_corrected);
    
    clamp_angle_to_limit(&yaw_corrected);
    clamp_angle_to_limit(&pitch_corrected);
    clamp_angle_to_limit(&roll_corrected);
    
    float current_clamp_limit = (float)imu_angle_clamp_limit;
    int16_t yaw_scaled = scale_angle_to_int16(yaw_corrected, -current_clamp_limit, current_clamp_limit);
    int16_t pitch_scaled = scale_angle_to_int16(pitch_corrected, -current_clamp_limit, current_clamp_limit);
    int16_t roll_scaled = scale_angle_to_int16(roll_corrected, -current_clamp_limit, current_clamp_limit);
    uint16_t magnitude_scaled = scale_magnitude_to_uint16(hp_magnitude, 25.0f);
    uint16_t mic_level = mic_level_get();
    
    if (has_magnetometer) {
        imu_report_9dof_t imu_report = {
            .yaw = yaw_scaled,
            .pitch = pitch_scaled,
            .roll = roll_scaled,
            .magnitude = magnitude_scaled,
            .mic_level = mic_level
        };
        handle_received_report((uint8_t*)&imu_report, (int)sizeof(imu_report), IMU_VIRTUAL_INTERFACE);
    } else {
        imu_report_6dof_t imu_report = {
            .pitch = pitch_scaled,
            .roll = roll_scaled,
            .magnitude = magnitude_scaled,
            .mic_level = mic_level
        };
        handle_received_report((uint8_t*)&imu_report, (int)sizeof(imu_report), IMU_VIRTUAL_INTERFACE);
    }

    activity_led_flash(LED_ACTIVITY_DURATION_MS);


    k_work_reschedule(&imu_work, K_MSEC(IMU_SAMPLE_RATE_MS));
}

bool sensor_inputs_init() {
    if (!motion_sensor_init(&has_magnetometer)) {
        return false;
    }
    
    imu_config.angle_clamp_limit = (float)imu_angle_clamp_limit;
    orientation_offset_initialized = false;
    yaw_reference_initialized = false;
    motion_fusion_reset();
    motion_fusion_reset_mag_calibration();
    reset_orientation_filters();

    float ax, ay, az, gx, gy, gz;
    if (!motion_sensor_read_imu(&ax, &ay, &az, &gx, &gy, &gz)) {
        return false;
    }

    if (has_magnetometer) {
        parse_descriptor(0x0F0D, 0x00C1, imu_hid_report_desc_9dof, IMU_HID_REPORT_DESC_9DOF_SIZE, IMU_VIRTUAL_INTERFACE, 0);
    } else {
        parse_descriptor(0x0F0D, 0x00C1, imu_hid_report_desc_6dof, IMU_HID_REPORT_DESC_6DOF_SIZE, IMU_VIRTUAL_INTERFACE, 0);
    }
    device_connected_callback(IMU_VIRTUAL_INTERFACE, 0x0F0D, 0x00C1, 0);
    
    their_descriptor_updated = true;


    k_work_schedule(&imu_work, K_MSEC(IMU_SAMPLE_RATE_MS));

    return true;
}

void sensor_inputs_recalibrate_orientation() {
    if (is_calibrated) {
        orientation_offset_initialized = false;
        reset_orientation_filters();

    }
}

void sensor_inputs_recalibrate_sensors() {
    if (is_calibrated) {
        is_calibrated = false;
        error_count = 0;
        
        motion_fusion_reset();
        orientation_offset_initialized = false;
        yaw_reference_initialized = false;
        motion_fusion_reset_mag_calibration();
        
        magnitude_filter = (iir_t){.y = 9.81f, .alpha = imu_config.magnitude_filter_alpha};
        mic_level_reset();
        reset_orientation_filters();

    }
}

#else

bool sensor_inputs_init() {
    return true;
}

#endif
