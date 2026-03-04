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
    bool is_connected();

private:
    static void message_arrived_callback(MQTT::MessageData& md);
    void handle_command(const char* payload);
    void publish_response(const char* command, const char* result, const char* message);

    GarageDoor& _door;
    MQTT::Client<IPStack, Countdown> _client;
    IPStack& _ipstack;

    // MQTT topics
    static constexpr const char* STATUS_TOPIC = "garage/door/status";
    static constexpr const char* COMMAND_TOPIC = "garage/door/command";
    static constexpr const char* RESPONSE_TOPIC = "garage/door/response";

    // Access instances of the class in static callbacks
    static MqttController* _instance;
};

#endif //PICO_MODBUS_MQTTCONTROLLER_H