#if defined(INIT_EEPROM)

#include <Arduino.h>
#include "storage.h"

void setup()
{
    Serial.begin(115200);
    delay(100);

    storage::init();

    storage::setDeviceID("switch-qube-1");
    storage::setRelayStatus(1);

    char device_id[32] = "";
    storage::getDeviceID(device_id);
    Serial.print("Device ID = ");
    Serial.println(device_id);

    int relay_status = 0;
    storage::getRelayStatus(&relay_status);
    Serial.print("Relay Status = ");
    Serial.println(relay_status);

    Serial.println("END OF EEPROM INITIALIZATION");
}

void loop()
{

}

#endif