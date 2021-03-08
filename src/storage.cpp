#include "storage.h"

storage::AbstractedStorage localStorage;


void storage::AbstractedStorage::begin(int size) { EEPROM.begin(size); }

void storage::AbstractedStorage::readString(int address, char *data) {
    char temp[32];
    EEPROM.get(address, temp);
    strcpy(data, temp);
}

std::string storage::AbstractedStorage::readString(int address) {
    char temp[32];
    EEPROM.get(address, temp);
    std::string final_string(temp);
    return final_string;
}

bool storage::AbstractedStorage::writeString(int address, std::string data) {
    short length_of_array = data.length();
    const char *data_array = data.c_str();

    int i = 0;
    for (i = 0; i < length_of_array; i++) {
        EEPROM.write(address + i, data_array[i]);
    }
    EEPROM.write(address + i, '\0');
    bool a = EEPROM.commit();
    delay(200);
    return a;
}

uint8_t storage::AbstractedStorage::readByte(int address) {
    return EEPROM.read(address);
}

void storage::AbstractedStorage::writeByte(int address, uint8_t data) {
    EEPROM.write(address, data);
}

void storage::AbstractedStorage::commit() { EEPROM.commit(); }

void storage::init() {
#ifndef UNIT_TEST
    Serial.print("Initialising local storage...");
#endif

    storage.begin(EEPROM_SIZE);

    // Check for EEPROM Initialization
    if (storage.readByte(EEPROM_INIT_ADDR) == EEPROM_INIT_CHECK_VALUE) {
#ifndef UNIT_TEST
        Serial.println("EEPROM has already been initialized.");
#endif
    } else {
#ifndef UNIT_TEST
        Serial.println("EEPROM is not initialized.");
#endif
    }
}

bool storage::setDeviceID(std::string device_id) {
    return localStorage.writeString(DEVICE_ID_START, device_id);
}

void storage::getDeviceID(char *data) {
    char t[32];
    EEPROM.get(DEVICE_ID_START, t);
    strcpy(data, t);
}

bool storage::setRelayStatus(int state) {
    EEPROM.put(RELAY_STATUS_START, state);
    bool a = EEPROM.commit();
    delay(200);
    return a;
}

void storage::getRelayStatus(int *state) {
    EEPROM.get(RELAY_STATUS_START, *state);
}