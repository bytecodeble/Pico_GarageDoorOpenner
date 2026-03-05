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
#include "SecretConfig.h"

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

// #define USE_MQTT

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
#ifdef USE_MQTT
    const char *topic = "garage/door";
    IPStack ipstack(WIFI_SSID, WIFI_PASSWORD);
    //MqttController mqtt(door, ipstack);
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect(MQTT_BROKER_IP, 1883);
    if (rc != 1) {
        printf("rc from TCP connect is %d\n", rc);
    }

    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *) "PicoW-GarageDoor";
    data.username.cstring = (char *)"yly";
    data.password.cstring = (char *)"test";


    rc = client.connect(data);
    if (rc != 0) {
        printf("rc from MQTT connect is %d\n", rc);
        while (true) {
            tight_loop_contents();
        }
    }
    printf("MQTT connected\n");

    // We subscribe QoS2. Messages sent with lower QoS will be delivered using the QoS they were sent with
    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("rc from MQTT subscribe is %d\n", rc);
    }
    printf("MQTT subscribed\n");

    auto mqtt_send = make_timeout_time_ms(2000);
    int mqtt_qos = 0;
    int msg_count = 0;
#endif


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






    while (true) {
        // Check for calibration command SW0+SW2
        if (!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2)) {
            sleep_ms(100); // debounce
            if (!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2)) {
                door.start_calibration();
                // Wait until buttons are released
                while(!gpio_get(BUTTON_CAL_1) && !gpio_get(BUTTON_CAL_2));
            }
        }

        // Check for operate command  SW1
        if (!gpio_get(BUTTON_OPERATE)) {
            sleep_ms(100); // debounce
            if (!gpio_get(BUTTON_OPERATE)) {
                door.operate();
                // Wait until button is released
                while(!gpio_get(BUTTON_OPERATE));
            }
        }

        // update the state machine
        door.update();

        // update led if stuck
        door.update_leds();
        sleep_ms(2);
    

#ifdef USE_MQTT
        if (time_reached(mqtt_send)) {
            mqtt_send = delayed_by_ms(mqtt_send, 2000);
            if (!client.isConnected()) {
                printf("Not connected...\n");
                rc = client.connect(data);
                if (rc != 0) {
                    printf("rc from MQTT connect is %d\n", rc);
                }

            }
            char buf[100];
            int rc = 0;
            MQTT::Message message;
            message.retained = false;
            message.dup = false;
            message.payload = (void *) buf;
            switch (mqtt_qos) {
                case 0:
                    // Send and receive QoS 0 message
                    sprintf(buf, "Msg nr: %d QoS 0 message", ++msg_count);
                    printf("%s\n", buf);
                    message.qos = MQTT::QOS0;
                    message.payloadlen = strlen(buf) + 1;
                    rc = client.publish(topic, message);
                    printf("Publish rc=%d\n", rc);
                    ++mqtt_qos;
                    break;
                case 1:
                    // Send and receive QoS 1 message
                    sprintf(buf, "Msg nr: %d QoS 1 message", ++msg_count);
                    printf("%s\n", buf);
                    message.qos = MQTT::QOS1;
                    message.payloadlen = strlen(buf) + 1;
                    rc = client.publish(topic, message);
                    printf("Publish rc=%d\n", rc);
                    ++mqtt_qos;
                    break;
#if MQTTCLIENT_QOS2
                    case 2:
                        // Send and receive QoS 2 message
                        sprintf(buf, "Msg nr: %d QoS 2 message", ++msg_count);
                        printf("%s\n", buf);
                        message.qos = MQTT::QOS2;
                        message.payloadlen = strlen(buf) + 1;
                        rc = client.publish(topic, message);
                        printf("Publish rc=%d\n", rc);
                        ++mqtt_qos;
                        break;
#endif
                default:
                    mqtt_qos = 0;
                    break;
            }
        }

        cyw43_arch_poll(); // obsolete? - see below
        client.yield(100); // socket that client uses calls cyw43_arch_poll()
#endif

        //tight_loop_contents();
    }
}

