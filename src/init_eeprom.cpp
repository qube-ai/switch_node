#if defined(INIT_EEPROM)

    #include <Arduino.h>
    #include <EEPROM.h>

    #define EEPROM_INIT_CHECK_ADDR 0
    #define RELAY_STATE_ADDR 1
    #define DEVICE_ID_ADDR_START 2
    #define DEVICE_ID "switch-qube-1"
    #define DEVICE_ID_ADDR_END 22


void setup() {
    Serial.begin(115200);
    delay(100);

    EEPROM.begin(512);
    Serial.println("EEPROM initialized");

    // Default Relay State
    EEPROM.write(RELAY_STATE_ADDR, 1);
    Serial.println("Default relay state written");

    // Name of the device
    String sample = DEVICE_ID;
    for (int i = DEVICE_ID_ADDR_START; i < sample.length(); i++) {
        EEPROM.write(i, sample[i]);
    }

    // EEPROM init complete
    EEPROM.write(EEPROM_INIT_CHECK_ADDR, 1);

    EEPROM.commit();
    Serial.println("EEPROM INIT complete.");
}

void loop() {}

#endif