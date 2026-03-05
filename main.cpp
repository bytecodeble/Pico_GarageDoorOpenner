#include <stdio.h>
#include <string.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "uart/PicoUart.h"

#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"

#include "PicoI2CDevice.h"

// project headers
#include "GarageDoor.h"
#include "StepperMotor.h"
#include "RotaryEncoder.h"
#include "MqttController.h"

// wifi ssid and password header file
#include "SecretConfig.h"

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 1 // for simulator
//#define STOP_BITS 2 // for real system

#define USE_MQTT

void messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;

    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n",
           message.qos, message.retained, message.dup, message.id);
    printf("Payload %s\n", (char *) message.payload);
}


int main() {
    // Initialize chosen serial port
    stdio_init_all();
    sleep_ms(2000);
    printf("\nBoot\n");

    // step motor pins
    const uint STEPPERS[4] = {2, 3, 6, 13};
    // encoder
    const uint ENCODER_A = 16;
    const uint ENCODER_B = 17;
    // limit switch
    const uint LIMIT_TOP = 28;
    const uint LIMIT_BOTTOM = 27;
    // buttons and led
    const uint BUTTON_OPERATE = 8; // SW1
    const uint BUTTON_CAL_1 = 9; // SW0
    const uint BUTTON_CAL_2 = 7; // SW2
    const uint LED_ERROR = 20; // For blinking on error

    // Initialize hardware
    StepperMotor motor(STEPPERS);
    RotaryEncoder encoder(ENCODER_A, ENCODER_B);
    GarageDoor door(motor, encoder, LIMIT_TOP, LIMIT_BOTTOM, LED_ERROR);

    // Initialize UI pins
    gpio_init(BUTTON_OPERATE);
    gpio_set_dir(BUTTON_OPERATE, GPIO_IN);
    gpio_pull_up(BUTTON_OPERATE);

    gpio_init(BUTTON_CAL_1);
    gpio_set_dir(BUTTON_CAL_1, GPIO_IN);
    gpio_pull_up(BUTTON_CAL_1);

    gpio_init(BUTTON_CAL_2);
    gpio_set_dir(BUTTON_CAL_2, GPIO_IN);
    gpio_pull_up(BUTTON_CAL_2);

    gpio_init(LED_ERROR);
    gpio_set_dir(LED_ERROR, GPIO_OUT);

    printf("Initialization complete. Waiting for commands.\n");
    printf("Press SW0 and SW2 to start calibration.\n");

#ifdef USE_MQTT
    // MQTT initialization
    IPStack ipstack(WIFI_SSID, WIFI_PASSWORD);
    printf("IPStack initialized\n");
    MqttController mqtt(door, ipstack);
    mqtt.connect();
    if (!mqtt.is_connected()) {
        printf("MQTT initialization failed\n");
    }

    auto mqtt_status_timer = make_timeout_time_ms(5000);
    DoorState last_reported_state = door.get_state();
#endif


    while (true) {
        // Check for calibration command SW0+SW2
        if (!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2)) {
            sleep_ms(100); // debounce
            if (!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2)) {
                door.start_calibration();
#ifdef USE_MQTT
                mqtt.reconnect();
                mqtt.publish_status();
#endif
                // Wait until buttons are released
                while(!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2));
            }
        }

        // Check for operate command  SW1
        if (!gpio_get(BUTTON_OPERATE)) {
            sleep_ms(100); // debounce
            if (!gpio_get(BUTTON_OPERATE)) {
                door.operate();
#ifdef USE_MQTT
                mqtt.publish_status();
#endif
                // Wait until button is released
                while(!gpio_get(BUTTON_OPERATE));
            }
        }

        // update the state machine
        door.update();

        // update led if stuck
        door.update_leds();

#ifdef USE_MQTT

        if (mqtt.has_pending_calibrate()) {
            mqtt.clear_pending_calibrate();
            door.start_calibration();
            mqtt.reconnect();

            if (door.get_state() == DoorState::ERROR_STUCK) {
                mqtt.publish_response("CALIBRATE", "ERROR", "Door got stuck during calibration.");
            } else {
                mqtt.publish_response("CALIBRATE", "SUCCESS", "Calibration complete.");
            }
            mqtt.publish_status();
        }

        if (mqtt.has_pending_operate()) {
            mqtt.clear_pending_operate();

            if (!door.is_calibrated()) {
                mqtt.publish_response("OPERATE", "ERROR", "Door not calibrated.");
            } else {
                door.operate();
                mqtt.publish_response("OPERATE", "SUCCESS", "Operated.");
            }

            mqtt.publish_status();
        }

#endif
#ifdef USE_MQTT
        // publish status on change
        if (door.get_state() != last_reported_state) {
            last_reported_state = door.get_state();
            mqtt.publish_status();
        }

        if (time_reached(mqtt_status_timer)) {
            mqtt_status_timer = delayed_by_ms(mqtt_status_timer, 5000);
            mqtt.publish_status();
        }

        // reconnect if disconnected
        if (!mqtt.is_connected()) {
            mqtt.connect();
        }

        if (door.get_state() == DoorState::OPENING || door.get_state() == DoorState::CLOSING) {
            mqtt.yield(1);
        } else {
            mqtt.yield(10);
        }

#endif
    }
}

