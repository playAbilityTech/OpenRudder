#include "mic_level.h"

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#define MIC_NODE DT_NODELABEL(pdm0)
#define MIC_LEVEL_AVAILABLE (DT_NODE_HAS_STATUS(MIC_NODE, okay) && IS_ENABLED(CONFIG_AUDIO_DMIC_NRFX_PDM))

#if MIC_LEVEL_AVAILABLE
#include <zephyr/audio/dmic.h>
#endif

#define MIC_SAMPLE_RATE_HZ 16000
#define MIC_SAMPLE_BIT_WIDTH 16
#define MIC_BLOCK_SAMPLES 160
#define MIC_BLOCK_SIZE (MIC_BLOCK_SAMPLES * sizeof(int16_t))
#define MIC_BLOCK_COUNT 4
#define MIC_MIN_PDM_CLK_FREQ 1000000
#define MIC_MAX_PDM_CLK_FREQ 3500000
#define MIC_MIN_PDM_CLK_DC 40
#define MIC_MAX_PDM_CLK_DC 60
#define MIC_LEVEL_MAX 120.0f
#define MIC_LEVEL_FILTER_ALPHA 0.65f
#define MIC_THREAD_STACK_SIZE 2048
#define MIC_THREAD_PRIORITY K_PRIO_COOP(2)
#define MIC_READ_TIMEOUT_MS 1000
#define MIC_MAX_READ_ERRORS 3
#define MIC_RESTART_DELAY_MS 20

typedef struct {
    float y;
    float alpha;
} iir_t;

static atomic_t latest_mic_level = ATOMIC_INIT(0);

#if MIC_LEVEL_AVAILABLE
static const struct device* mic_dev = DEVICE_DT_GET(MIC_NODE);
K_MEM_SLAB_DEFINE_STATIC(mic_mem_slab, MIC_BLOCK_SIZE, MIC_BLOCK_COUNT, 4);
K_THREAD_STACK_DEFINE(mic_thread_stack, MIC_THREAD_STACK_SIZE);
static struct k_thread mic_thread;
static iir_t mic_level_filter = {.y = 0.0f, .alpha = MIC_LEVEL_FILTER_ALPHA};
static bool mic_configured = false;
static bool mic_streaming = false;
static bool mic_thread_started = false;

static uint16_t scale_mic_level_to_uint16(float level) {
    level = fmaxf(0.0f, fminf(MIC_LEVEL_MAX, level));
    return (uint16_t)(level + 0.5f);
}

static float compute_mic_level_from_pcm(const int16_t* samples, size_t sample_count) {
    if (sample_count == 0) {
        return 0.0f;
    }

    double sum = 0.0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < sample_count; i++) {
        double sample = samples[i];
        sum += sample;
        sum_sq += sample * sample;
    }

    double mean = sum / sample_count;
    double variance = sum_sq / sample_count - mean * mean;
    if (variance <= 1.0) {
        return 0.0f;
    }

    return 20.0f * log10f((float)sqrt(variance));
}

static void mic_free_buffer(void* buffer) {
    k_mem_slab_free(&mic_mem_slab, &buffer);
}

static bool mic_configure(void) {
    if (mic_configured) {
        return true;
    }

    if (!device_is_ready(mic_dev)) {
        return false;
    }

    static struct pcm_stream_cfg stream = {
        .pcm_rate = MIC_SAMPLE_RATE_HZ,
        .pcm_width = MIC_SAMPLE_BIT_WIDTH,
        .block_size = MIC_BLOCK_SIZE,
        .mem_slab = &mic_mem_slab,
    };

    struct dmic_cfg cfg = {
        .io = {
            .min_pdm_clk_freq = MIC_MIN_PDM_CLK_FREQ,
            .max_pdm_clk_freq = MIC_MAX_PDM_CLK_FREQ,
            .min_pdm_clk_dc = MIC_MIN_PDM_CLK_DC,
            .max_pdm_clk_dc = MIC_MAX_PDM_CLK_DC,
        },
        .streams = &stream,
        .channel = {
            .req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT),
            .req_num_chan = 1,
            .req_num_streams = 1,
        },
    };

    if (dmic_configure(mic_dev, &cfg) < 0) {
        return false;
    }

    mic_configured = true;
    return true;
}

static bool mic_start_stream(void) {
    if (!mic_configure()) {
        return false;
    }

    if (mic_streaming) {
        return true;
    }

    if (dmic_trigger(mic_dev, DMIC_TRIGGER_START) < 0) {
        return false;
    }

    mic_streaming = true;
    return true;
}

static void mic_stop_stream(void) {
    if (mic_configured && mic_streaming) {
        dmic_trigger(mic_dev, DMIC_TRIGGER_STOP);
    }
    mic_streaming = false;
}

static void mic_drain_available_buffers(void) {
    while (true) {
        void* buffer = NULL;
        size_t size = 0;
        int err = dmic_read(mic_dev, 0, &buffer, &size, 0);
        if (err < 0 || buffer == NULL || size == 0) {
            break;
        }

        mic_free_buffer(buffer);
    }
}

static void mic_update_level(const int16_t* samples, size_t sample_count) {
    float raw_level = compute_mic_level_from_pcm(samples, sample_count);
    mic_level_filter.y = mic_level_filter.alpha * mic_level_filter.y +
                         (1.0f - mic_level_filter.alpha) * raw_level;
    atomic_set(&latest_mic_level, (atomic_val_t)scale_mic_level_to_uint16(mic_level_filter.y));
}

static void mic_thread_fn(void* arg1, void* arg2, void* arg3) {
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint8_t read_errors = 0;

    while (true) {
        if (!mic_start_stream()) {
            mic_level_reset();
            k_msleep(MIC_RESTART_DELAY_MS);
            continue;
        }

        void* buffer = NULL;
        size_t size = 0;
        int err = dmic_read(mic_dev, 0, &buffer, &size, MIC_READ_TIMEOUT_MS);
        if (err == 0 && buffer != NULL && size > 0) {
            read_errors = 0;
            mic_update_level((const int16_t*)buffer, size / sizeof(int16_t));
            mic_free_buffer(buffer);
            continue;
        }

        if (buffer != NULL) {
            mic_free_buffer(buffer);
        }

        if (++read_errors >= MIC_MAX_READ_ERRORS) {
            mic_stop_stream();
            mic_drain_available_buffers();
            mic_level_reset();
            read_errors = 0;
            k_msleep(MIC_RESTART_DELAY_MS);
        }
    }
}

bool mic_level_init(void) {
    if (mic_thread_started) {
        return true;
    }

    if (!mic_configure()) {
        return false;
    }

    k_thread_create(&mic_thread, mic_thread_stack, K_THREAD_STACK_SIZEOF(mic_thread_stack),
                    mic_thread_fn, NULL, NULL, NULL,
                    MIC_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&mic_thread, "mic_level");
    mic_thread_started = true;
    return true;
}

#else

bool mic_level_init(void) {
    return false;
}

#endif

uint16_t mic_level_get(void) {
    return (uint16_t)atomic_get(&latest_mic_level);
}

void mic_level_reset(void) {
    atomic_set(&latest_mic_level, 0);
}
