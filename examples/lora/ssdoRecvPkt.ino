
#include <radio.h>
#include <WiFi.h>

#define NSS  18
#define DIO0 26
#define NRST 23

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

// init HW level radioLib
RADIOHW radio = new Module(NSS, DIO0, NRST);

//declare radioControl (SOFT Layer)
RadioControl* radioControl;

void packetReader(uint8_t * body, ssdoHeader_t * header) {
    DEBUG_PRINT("Readed lora SSDO packet id: ", header->pktId, " size: ", header->pktSize);
    DEBUG_PRINT_HEX(body, header->pktSize);
}

void setup() {
    WiFi.persistent(false);                  //disable saving wifi config into SDK flash area
    WiFi.mode(WIFI_OFF);                     //disable swAP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
    
    DEBUG_BEGIN();

    radioControl = new RadioControl(&radio); //init radio control

    radioControl->setupLora(loraTelemetry); //setup lora
    radioControl->setSSDOSender(0xFAFA); // set sender ID for SSDO (some random UINT32)
    radioControl->setLoraSSDOPacketHandler(packetReader); // set radio in recive mode
}

void loop() {
    radioControl->processRecvBuff(); // process recv buff
}
