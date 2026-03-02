#include "RotaryEncoder.h"

// Static members
uint RotaryEncoder::_pin_a;
uint RotaryEncoder::_pin_b;
queue_t RotaryEncoder::_event_queue;

RotaryEncoder::RotaryEncoder(uint pin_a, uint pin_b) {
    _pin_a = pin_a;
    _pin_b = pin_b;

    // Initialize queue for communication between ISR and main
    queue_init(&_event_queue, sizeof(Rotation), 16);

    // Configure GPIOs
    gpio_init(_pin_a);
    gpio_set_dir(_pin_a, GPIO_IN);

    gpio_init(_pin_b);
    gpio_set_dir(_pin_b, GPIO_IN);

    // Set up the interrupt
    gpio_set_irq_enabled_with_callback(_pin_a, GPIO_IRQ_EDGE_RISE, true, &RotaryEncoder::gpio_callback);
}

// Interrupt Service Routine (ISR)
// It must be a static function
void RotaryEncoder::gpio_callback(uint gpio, uint32_t events) {
    Rotation rotation_event;
    if (gpio_get(_pin_b)) {
        // B is high on rising edge of A -> Counter-Clockwise
        rotation_event = Rotation::COUNTER_CLOCKWISE;
    } else {
        // B is low on rising edge of A -> Clockwise
        rotation_event = Rotation::CLOCKWISE;
    }
    // Add the event to the queue. Use 'try' to avoid blocking in an ISR.
    queue_try_add(&_event_queue, &rotation_event);
}

// This function is called from the main loop to get rotation events
Rotation RotaryEncoder::get_rotation() {
    Rotation received_rotation = Rotation::NONE;
    // Check if there is an event in the queue
    if (queue_try_remove(&_event_queue, &received_rotation)) {
        return received_rotation;
    }
    return Rotation::NONE;
}