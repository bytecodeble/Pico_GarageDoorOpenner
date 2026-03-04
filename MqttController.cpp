#include "MqttController.h"
#include "SecretConfig.h"
#include <stdio.h>
#include <string.h>

MqttController* MqttController::_instance = nullptr;

MqttController::MqttController(GarageDoor& door, IPStack& ipstack)
    : _door(door), _ipstack(ipstack), _client(ipstack) {
    _instance = this;
}

bool MqttController::connect() {
    int rc = _ipstack.connect(MQTT_BROKER_IP, 1883); // !!注意替换成实际的MQTT代理地址!!
    if (rc != 1) {
        printf("MQTT TCP connect failed with rc %d\n", rc);
        return false;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*)"PicoW-GarageDoor";

    rc = _client.connect(data);
    if (rc != 0) {
        printf("MQTT connect failed with rc %d\n", rc);
        return false;
    }

    printf("MQTT connected.\n");

    // 订阅命令主题
    rc = _client.subscribe(COMMAND_TOPIC, MQTT::QOS0, message_arrived_callback);
    if (rc != 0) {
        printf("MQTT subscribe failed with rc %d\n", rc);
        return false;
    }
    printf("MQTT subscribed to %s\n", COMMAND_TOPIC);
    return true;
}

void MqttController::message_arrived_callback(MQTT::MessageData& md) {
    if (_instance) {
        MQTT::Message& message = md.message;
        char payload_str[message.payloadlen + 1];
        memcpy(payload_str, message.payload, message.payloadlen);
        payload_str[message.payloadlen] = '\0';
        
        printf("MQTT message arrived on topic %.*s: %s\n", md.topicName.lenstring.len, md.topicName.lenstring.data, payload_str);
        _instance->handle_command(payload_str);
    }
}

void MqttController::handle_command(const char* payload) {
    if (strcmp(payload, "OPERATE") == 0) {
        if (_door.is_calibrated()) {
            _door.operate();
            publish_response("OPERATE", "SUCCESS", "Command received.");
        } else {
            publish_response("OPERATE", "ERROR", "Door not calibrated.");
        }
    } else if (strcmp(payload, "CALIBRATE") == 0) {
        _door.start_calibration();
        publish_response("CALIBRATE", "SUCCESS", "Calibration started.");
    } else {
        publish_response(payload, "ERROR", "Unknown command.");
    }
    // commands may change state, publish new state
    publish_status();
}

void MqttController::publish_status() {
    if (!_client.isConnected()) return;

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"door_state\": \"%s\", \"error_state\": \"%s\", \"calibration_state\": \"%s\"}",
             _door.get_state_string(),
             _door.get_state() == DoorState::ERROR_STUCK ? "DOOR_STUCK" : "NORMAL",
             _door.is_calibrated() ? "CALIBRATED" : "NOT_CALIBRATED");

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = true; // keep message for client to get status immediately
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);

    _client.publish(STATUS_TOPIC, message);
}

void MqttController::publish_response(const char* command, const char* result, const char* message_text) {
    if (!_client.isConnected()) return;

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"command\": \"%s\", \"result\": \"%s\", \"message\": \"%s\"}",
             command, result, message_text);

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);

    _client.publish(RESPONSE_TOPIC, message);
}

void MqttController::yield(int time_ms) {
    _client.yield(time_ms);
}

bool MqttController::is_connected() {
    return _client.isConnected();
}