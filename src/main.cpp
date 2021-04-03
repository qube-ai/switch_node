#if defined(RELEASE_CODE)
    #include "Arduino.h"
    // This is to prevent clang-format to reorder includes
    // which breaks the code
    #include "namedMesh.h"
    //
    #include <EEPROM.h>
    //
    #include "ArduinoJson.h"
    //
    #include "storage.h"

    #define MESH_PREFIX "qubeMeshNet"
    #define MESH_PASSWORD "iridiumcoresat12"
    #define MESH_PORT 5555

    /* GPIO pins in use */
    #define RELAY_PIN 0
    #define SWITCH_PIN 2
    #define FIRMWARE_VERSION "1.0.0"

Scheduler userScheduler;
namedMesh mesh;
// SoftwareSerial debugSerial(0, 2);  // 0 - Rx, 2 - Tx

enum switch_state { ON = 1, OFF = 0 };
int switch_pin_state = OFF;

enum switch_state getSwitchState() {
    // Since the input is PULLED_HIGH.
    // HIGH means Switch is off
    // LOW means switch is on
    short t = digitalRead(SWITCH_PIN);
    if (t == LOW) {
        return ON;
    } else {
        return OFF;
    }
}

inline void actuateRelay(short value) {
    // Relay is active low
    digitalWrite(RELAY_PIN, !value);
}

void sendStateMessage() {
    // Create a message and send it to master
    Serial.print("Sending state message...");
    StaticJsonDocument<120> doc;
    String doc_string;
    String to_node = "master";

    int current_priority = 0;
    storage::getPriority(&current_priority);

    int current_relay_state = 0;
    storage::getRelayStatus(&current_relay_state);

    doc["t"] = 1;
    doc["ss"] = getSwitchState();
    doc["rs"] = current_relay_state;
    doc["p"] = current_priority;
    doc["fw"] = FIRMWARE_VERSION;

    serializeJson(doc, doc_string);
    mesh.sendSingle(to_node, doc_string);
    Serial.println("SENT");

    Serial.print("State messages was -> ");
    Serial.println(doc_string);
}

void switch_pin_watcher() {
    // Get the current state of switch pin
    int current_state = getSwitchState();

    // Compare current state with last known state to see if it has changed
    if (current_state != switch_pin_state) {
        // If the switch is ON(1) then switch on the relay
        // If the switch is OFF(0) then switch off the relay
        int relay_state = (current_state ? HIGH : LOW);
        Serial.printf("Physical switch says put relay to -> %d\n", relay_state);
        storage::setRelayStatus(relay_state);

        // Physical switch is state change is accepted only in priority 0 and 1,
        // ignore everything else
        int priority = 0;
        storage::getPriority(&priority);
        if (priority == 0 || priority == 1) {
            actuateRelay(relay_state);

            // Send a state message only when the relay is actuated
            sendStateMessage();

        } else {
            Serial.printf(
                "Current priority(%d) doesn't allow for relay state change.\n",
                priority);
        }

        switch_pin_state = current_state;
    }
}
// Execute this task every 250ms
Task switch_pin_watcher_task(250ul, TASK_FOREVER, &switch_pin_watcher);

void heartbeat_signal() { sendStateMessage(); }
// Send a heartbeat signal every 5 minutes
Task heartbeat_task(300000ul, TASK_FOREVER, &heartbeat_signal);

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // What type of message is it
    short type = doc["t"];

    // Type 1 message
    if (type == 1) {
        int required_relay_status = doc["rs"];
        int required_priority = doc["p"];

        bool relay_state_changed = false;
        bool priority_changed = false;

        // Fetch current state of relay
        int current_relay_status = 0;
        storage::getRelayStatus(&current_relay_status);

        // Fetch current priority
        int current_priority = 0;
        storage::getPriority(&current_priority);

        // Check if we need to update the relay state.

        if (current_priority == 1 || current_priority == 2) {
            storage::setRelayStatus(required_relay_status);
            actuateRelay(required_relay_status);
            Serial.println(
                "Relay state was updated due to new config. Relay -> " +
                String(required_relay_status));
            relay_state_changed = true;
        }

        else {
            Serial.printf(
                "Current priority(%d) doesn't allow for relay state "
                "change.\n",
                current_priority);
        }

        // update priority if required
        if (current_priority != required_priority) {
            storage::setPriority(required_priority);
            Serial.println("Device priority was updated due to new config.");
            priority_changed = true;
        }

        // Check if we require to send a state message
        if (priority_changed || relay_state_changed) { sendStateMessage(); }

    }

    else {
        Serial.println("Unknown type of message received.");
    }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() { Serial.printf("Changed connections\n"); }

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),
                  offset);
}

void handleInterrupt() { Serial.println("Interrupt Detected"); }

void setup() {
    // Setup serial connection for debugging
    Serial.begin(115200);
    EEPROM.begin(512);

    // Initialize mesh
    mesh.setDebugMsgTypes(ERROR | STARTUP);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    mesh.initOTAReceive(
        "SW2-30A");  // Set role of this node for receiving OTA updates.
    Serial.println("Mesh properties set");
    Serial.printf("Mesh properties set for ssid -> %s and pass -> %s\n",
                  MESH_PREFIX, MESH_PASSWORD);

    // Setup GPIO pins
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    switch_pin_state = getSwitchState();

    // Add a task for watching the GPIO pins
    userScheduler.addTask(switch_pin_watcher_task);
    switch_pin_watcher_task.enable();

    userScheduler.addTask(heartbeat_task);
    heartbeat_task.enable();

    // Fetch Relay state from storage
    int relay_state_from_storage = 0;
    storage::getRelayStatus(&relay_state_from_storage);
    Serial.printf("Relay state as per storage -> %d\n",
                  relay_state_from_storage);
    actuateRelay(relay_state_from_storage);

    // Set node name from storage
    char dev_id[20] = "";
    storage::getDeviceID(dev_id);
    Serial.printf("Device ID from storage is -> %s\n", dev_id);
    String this_node_name(dev_id);
    mesh.setName(this_node_name);

    Serial.println("Setup() complete");
}

void loop() {
    // it will run the user scheduler as well
    mesh.update();
}

#endif