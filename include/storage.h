#pragma once
#include <Arduino.h>
#include <EEPROM.h>

#include <string>

#define EEPROM_SIZE 512

#define EEPROM_INIT_ADDR 0
#define EEPROM_INIT_CHECK_VALUE 139

#define RELAY_STATUS_START 10
#define DEVICE_ID_START 20

namespace storage {

    class AbstractedStorage {
       public:
        void begin(int size);

        std::string readString(int address);
        bool writeString(int address, std::string data);

        void readString(int address, char *data);

        uint8_t readByte(int address);
        void writeByte(int address, uint8_t data);

        void commit();
    };

    extern AbstractedStorage storage;

    void init();

    // Getters for Device ID
    bool setDeviceID(std::string device_id);
    void getDeviceID(char *data);

    // Getter and Setter for last reset time for energy meter readings
    bool setRelayStatus(int state);
    void getRelayStatus(int *state);

}  // namespace storage