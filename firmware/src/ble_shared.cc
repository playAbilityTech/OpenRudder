#include "ble_shared.h"

queue_t ble_report_queue;
queue_t ble_descriptor_queue;
queue_t ble_disconnected_queue;

void ble_queues_init(void) {
    queue_init(&ble_report_queue, sizeof(struct ble_report_t), 16);
    queue_init(&ble_descriptor_queue, sizeof(struct ble_descriptor_t), 2);
    queue_init(&ble_disconnected_queue, sizeof(struct ble_disconnected_t), BLE_MAX_CENTRAL_CONNECTIONS);
}

bool ble_report_queue_try_add(const struct ble_report_t* report) {
    return queue_try_add(&ble_report_queue, report);
}

bool ble_descriptor_queue_try_add(const struct ble_descriptor_t* descriptor) {
    return queue_try_add(&ble_descriptor_queue, descriptor);
}

bool ble_disconnected_queue_try_add(uint8_t conn_idx) {
    struct ble_disconnected_t item = { .conn_idx = conn_idx };
    return queue_try_add(&ble_disconnected_queue, &item);
}
