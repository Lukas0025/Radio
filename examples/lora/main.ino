
#include <radio.h>
#include <WiFi.h>

// declare lora settings
const LoraSettings_t loraTelemetry = {
    .Frequency      = 434.126,
    .Bandwidth      = 20.8,
    .SpreadFactor   = 11,
    .CodeRate       = 8,
    .SyncWord       = 0x12,
    .Power          = 17,
    .CurrentLimit   = 100,
    .PreambleLength = 8,
    .Gain           = 0
};

//first init radiolib module (HW Layer)
RADIOHW radio = new Module();

//declare radioControl (SOFT Layer)
RadioControl radioControl;

const uint8_t packet[]   = "hello";
const unsigned packetLen = 5;

void setup() {
    WiFi.persistent(false);                  //disable saving wifi config into SDK flash area
    WiFi.mode(WIFI_OFF);                     //disable swAP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
    
    radioControl = new RadioControl(&radio); //init radio control
}

void loop() {
    //setup LORA
    radioControl.setupLora(loraTelemetry);

    //Send LORA packet
    //max packet size is 255 bytes (Size is in BYTES)
    radioControl.sendLora(packet, packetLen);
    
    delay(10000);
}
