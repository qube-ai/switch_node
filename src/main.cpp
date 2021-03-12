#if defined(RELEASE_CODE)
    #include "Arduino.h"
    #include "namedMesh.h"
    #include <EEPROM.h>
    #include "ArduinoJson.h"
    #include "storage.h"
    #include <SoftwareSerial.h>

    #define MESH_PREFIX "whateverYouLike"
    #define MESH_PASSWORD "somethingSneaky"
    #define MESH_PORT 5555

    /* GPIO pins in use */
    #define RELAY_PIN 1   // Tx pin
    #define SWITCH_PIN 3  // Rx pin

Scheduler userScheduler;
namedMesh mesh;
int switch_pin_state = HIGH;
SoftwareSerial debugSerial(0, 2);  // 0 - Rx, 2 - Tx

inline void actuateRelay(short value) {
    // Relay is active low
    digitalWrite(RELAY_PIN, value);
}

void switch_pin_watcher() {
    // Get the current state of switch pin
    int current_state = digitalRead(SWITCH_PIN);

    // Compare current state with last known state to see if it has changed
    if (current_state != switch_pin_state) {
        // Set the relay to whatever state the switch desires
        // Since the pin is INPUT_PULLUP. So,
        // Switch OPEN / OFF = HIGH
        // Switch CLOSE / ON = GND
        int relay_state = (current_state ? LOW : HIGH);
        debugSerial.printf("Physical switch says put relay to -> %d\n",
                           relay_state);
        storage::setRelayStatus(relay_state);

        // Physical switch is state change is accepted only in priority 0 and 1,
        // ignore everything else
        int priority = 0;
        storage::getPriority(&priority);
        if (priority == 0 || priority == 1) {
            actuateRelay(relay_state);
        } else {
            debugSerial.printf(
                "Current priority(%d) doesn't allow for relay state change.\n",
                priority);
        }

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
// Execute this task every 500ms
Task switch_pin_watcher_task(500ul, TASK_FOREVER, &switch_pin_watcher);


void receivedCallback(uint32_t from, String &msg) {
    debugSerial.printf("Received from %u msg=%s\n", from, msg.c_str());

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        debugSerial.print(F("deserializeJson() failed: "));
        debugSerial.println(error.f_str());
        return;
    }

    // What type of message is it
    short type = doc["t"];

    // Type 1 message
    if (type == 1) {
        int required_relay_status = doc["rs"];
        int required_priority = doc["p"];

        // Fetch current state of relay
        int current_relay_status = 0;
        storage::getRelayStatus(&current_relay_status);

        // Fetch current priority
        int current_priority = 0;
        storage::getPriority(&current_priority);

        // update relay state if required
        if (current_relay_status != required_relay_status) {
            if (current_priority == 1 || current_priority == 2) {
                storage::setRelayStatus(required_relay_status);
                actuateRelay(required_relay_status);
                debugSerial.println(
                    "Relay state was updated due to new config.");
            }

            else {
                debugSerial.printf(
                    "Current priority(%d) doesn't allow for relay state "
                    "change.\n",
                    current_priority);
            }
        }

        // update priority if required
        if (current_priority != required_priority) {
            storage::setPriority(required_priority);
            debugSerial.println(
                "Device priority was updated due to new config.");
        }
    }

    else {
        debugSerial.println("Unknown type of message received.");
    }
}

void newConnectionCallback(uint32_t nodeId) {
    debugSerial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
    debugSerial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    debugSerial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),
                       offset);
}

void handleInterrupt() { debugSerial.println("Interrupt Detected"); }

void setup() {
    debugSerial.begin(9600);
    EEPROM.begin(512);

    // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC |
    // COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    // set before init() so that you can see startup messages
    mesh.setDebugMsgTypes(ERROR | STARTUP);
    debugSerial.println("Mesh debug type set");

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    debugSerial.println("Mesh properties set");
    debugSerial.printf("Mesh properties set for ssid -> %s and pass -> %s\n",
                       MESH_PREFIX, MESH_PASSWORD);

    // Add a task for watching the GPIO pins
    userScheduler.addTask(switch_pin_watcher_task);
    switch_pin_watcher_task.enable();

    // Setup GPIO pins
    // GPIO 1 (TX) swap the pin to a GPIO.
    pinMode(1, FUNCTION_3);
    // GPIO 3 (RX) swap the pin to a GPIO.
    pinMode(3, FUNCTION_3);
    pinMode(RELAY_PIN, OUTPUT);

    pinMode(SWITCH_PIN, INPUT_PULLUP);
    switch_pin_state = digitalRead(SWITCH_PIN);

    // Fetch Relay state from storage
    int relay_state_from_storage = 0;
    storage::getRelayStatus(&relay_state_from_storage);
    debugSerial.printf("Relay state as per storage -> %d\n",
                       relay_state_from_storage);
    actuateRelay(relay_state_from_storage);

    // Set node name from storage
    char dev_id[20] = "";
    storage::getDeviceID(dev_id);
    debugSerial.printf("Device ID from storage is -> %s\n", dev_id);
    String this_node_name(dev_id);
    mesh.setName(this_node_name);

    debugSerial.println("Setup() complete");
}

void loop() {
    // it will run the user scheduler as well
    mesh.update();
}

#endif