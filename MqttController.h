#ifndef PICO_MODBUS_MQTTCONTROLLER_H
#define PICO_MODBUS_MQTTCONTROLLER_H

#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"
#include "GarageDoor.h"

class MqttController {
public:
    MqttController(GarageDoor& door, IPStack& ipstack);
    bool connect();
    void publish_status();
    void yield(int time_ms = 100);
    void reconnect();

    bool is_connected();
    bool has_pending_operate();
    bool has_pending_calibrate();
    void clear_pending_operate();
    void clear_pending_calibrate();

    void publish_response(const char* command, const char* result, const char* message);

private:
    static void message_arrived_callback(MQTT::MessageData& md);
    void handle_command(const char* payload);

    GarageDoor& _door;
    MQTT::Client<IPStack, Countdown> _client;
    IPStack& _ipstack;

    // MQTT topics
    static constexpr const char* STATUS_TOPIC = "garage/door/status";
    static constexpr const char* COMMAND_TOPIC = "garage/door/command";
    static constexpr const char* RESPONSE_TOPIC = "garage/door/response";

    // Access instances of the class in static callbacks
    static MqttController* _instance;

    // pending command
    volatile bool _pending_operate = false;
    volatile bool _pending_calibrate = false;

    absolute_time_t _last_connect_attempt;
};

#endif //PICO_MODBUS_MQTTCONTROLLER_H