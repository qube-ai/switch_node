#if defined(INIT_EEPROM)

#include <Arduino.h>
#include "storage.h"

void setup()
{
    Serial.begin(115200);
    delay(100);

    storage::init();

    storage::setDeviceID("SW2-30A-1");
    storage::setRelayStatus(0); // By default the appliance would be off
    storage::setPriority(1);    // Follow switch and also listen to cloud messages

    char device_id[32] = "";
    storage::getDeviceID(device_id);
    Serial.print("Device ID = ");
    Serial.println(device_id);

    int relay_status = 0;
    storage::getRelayStatus(&relay_status);
    Serial.print("Relay Status = ");
    Serial.println(relay_status);

    int p = 0;
    storage::getPriority(&p);
    Serial.print("Priority = ");
    Serial.println(p);

    Serial.println("END OF EEPROM INITIALIZATION");
}

void loop()
{

}

#endif