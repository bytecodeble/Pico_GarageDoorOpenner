#include "MqttController.h"
#include "SecretConfig.h"
#include <stdio.h>
#include <string.h>

MqttController* MqttController::_instance = nullptr;

MqttController::MqttController(GarageDoor& door, IPStack& ipstack)
    : _door(door), _ipstack(ipstack), _client(ipstack), _last_connect_attempt(nil_time){
    _instance = this;
}

bool MqttController::connect() {
    if (!is_nil_time(_last_connect_attempt) &&
        absolute_time_diff_us(_last_connect_attempt, get_absolute_time()) < 5000 * 1000) {
        return false; // too soon, skip
        }
    _last_connect_attempt = get_absolute_time();

    int rc = _ipstack.connect(MQTT_BROKER_IP, 1883);
    if (rc != 0) {
        printf("MQTT TCP connect failed with rc %d\n", rc);
        _ipstack.disconnect();
        return false;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.username.cstring = (char *)"iot101";
    data.password.cstring = (char *)"test";
    data.clientID.cstring = (char*)"PicoW-GarageDoor";

    rc = _client.connect(data);
    if (rc != 0) {
        printf("MQTT connect failed with rc %d\n", rc);
        return false;
    }

    printf("MQTT connected.\n");

    // subscribe to command topic
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
        printf("RAW payload bytes: %d\n", (int)message.payloadlen);
        for(int i = 0; i < message.payloadlen; i++) {
            printf("%02x ", ((char*)message.payload)[i]);
        }
        printf("\n");
        char payload_str[message.payloadlen + 1];
        memcpy(payload_str, message.payload, message.payloadlen);
        payload_str[message.payloadlen] = '\0';
        
        printf("MQTT message arrived on topic %.*s: %s\n", md.topicName.lenstring.len, md.topicName.lenstring.data, payload_str);
        _instance->handle_command(payload_str);
    }
}

void MqttController::handle_command(const char* payload) {
    if (strcmp(payload, "OPERATE") == 0) {
        _pending_operate = true;
    }

    else if (strcmp(payload, "CALIBRATE") == 0) {
        _pending_calibrate = true;
    }

    else if (strcmp(payload, "STATUS") == 0) {
        publish_status();
    }
}

void MqttController::publish_status() {
    if (!_client.isConnected()) return;

    const char* door_state;
    DoorState s = _door.get_state();
    if (s == DoorState::OPEN) door_state = "OPEN";
    else if (s == DoorState::CLOSED) door_state = "CLOSED";
    else door_state = "IN_BETWEEN";

    char payload[256];
    snprintf(payload, sizeof(payload),
        "{\"door_state\":\"%s\",\"error_state\":\"%s\",\"calibration_state\":\"%s\"}",
        door_state,
        s == DoorState::ERROR_STUCK ? "DOOR_STUCK" : "NORMAL",
        _door.is_calibrated() ? "CALIBRATED" : "NOT_CALIBRATED");

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = true;
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);
    message.dup = false;
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

void MqttController::reconnect() {
    _last_connect_attempt = nil_time;
    _ipstack.disconnect();
    connect();

}

bool MqttController::is_connected() {
    return _client.isConnected();
}

bool MqttController::has_pending_operate() {
    return _pending_operate;
}

bool MqttController::has_pending_calibrate() {
    return _pending_calibrate;
}

void MqttController::clear_pending_operate() {
    _pending_operate = false;
}

void MqttController::clear_pending_calibrate() {
    _pending_calibrate = false;
}