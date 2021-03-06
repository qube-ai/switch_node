#if defined(RELEASE_CODE)
    /*
    BABY QUBE
    */
    #include "Arduino.h"
    #include "namedMesh.h"
    #include <EEPROM.h>
    #include "ArduinoJson.h"

    #include "storage.h"

    #define MESH_PREFIX "whateverYouLike"
    #define MESH_PASSWORD "somethingSneaky"
    #define MESH_PORT 5555

Scheduler userScheduler;  // to control your personal task
namedMesh mesh;
int switch_pin_state = HIGH;

    /* User defined variables */
    #define RELAY_PIN 2
    #define SWITCH_PIN 0  // This is D1 on Node MCU board


void sendMessage() {
    // String msg = "Hello from node 2";
    // String to_node = "master";

    // msg += mesh.getNodeId();
    // mesh.sendSingle(to_node, msg);
    // taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void switch_pin_watcher() {
    int current_state = digitalRead(SWITCH_PIN);
    // Serial.printf("Switch state was changed to -> %d\n", current_state);
    if (current_state != switch_pin_state) {
        // Set the relay to whatever state the switch desires
        // Since the pin is INPUT_PULLUP. So,
        // Switch OPEN / OFF = HIGH
        // Switch CLOSE / ON = GND
        int relay_state = (current_state ? LOW : HIGH);
        Serial.printf("Physical switch says put relay to -> %d\n", relay_state);
        storage::setRelayStatus(relay_state);
        digitalWrite(RELAY_PIN, relay_state);

        // Create a message and send it to master
        StaticJsonDocument<50> doc;
        String doc_string;
        String to_node = "master";

        doc["t"] = 1;
        doc["ss"] = current_state;
        doc["rs"] = current_state;
        serializeJson(doc, doc_string);
        mesh.sendSingle(to_node, doc_string);

        switch_pin_state = current_state;
    }
}
Task switch_pin_watcher_task(
    500ul, TASK_FOREVER, &switch_pin_watcher);  // Execute this task every 500ms

// Needed for painless library
void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // Listen for master's address
    short type = doc["t"];

    // Update relay state
    if (type == 2) {
        short relay_state = doc["rs"];
        Serial.printf("Gateway says to put relay to -> %d\n", relay_state);
        storage::setRelayStatus(relay_state);
        digitalWrite(RELAY_PIN, (relay_state ? HIGH : LOW));
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
    Serial.begin(115200);
    EEPROM.begin(512);

    // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC |
    // COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes(
        ERROR |
        STARTUP);  // set before init() so that you can see startup messages
    Serial.println("Mesh debug type set");

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    Serial.println("Mesh properties set");
    Serial.printf("Mesh properties set for ssid -> %s and pass -> %s\n", MESH_PREFIX, MESH_PASSWORD);

    // Get the Master Node ID
    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();

    // userScheduler.addTask( get_master_node_id_task );
    // get_master_node_id_task.enable();

    // Perform GPIO setup
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    switch_pin_state = digitalRead(SWITCH_PIN);

    userScheduler.addTask(switch_pin_watcher_task);
    switch_pin_watcher_task.enable();

    // Fetch Relay state from storage
    int relay_state_from_storage = 0;
    storage::getRelayStatus(&relay_state_from_storage);
    Serial.printf("Relay state as per storage -> %d\n", relay_state_from_storage);
    digitalWrite(RELAY_PIN, relay_state_from_storage);

    // Fetch Device ID from storage
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