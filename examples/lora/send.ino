
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

const LoraSettings_t loraHSTelemetry = {
    .Frequency      = 434.126,
    .Bandwidth      = 250,
    .SpreadFactor   = 7,
    .CodeRate       = 6,
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

uint8_t  packet[]       = "hello";
uint8_t  longData[10000] = {'F'}; // 10k bytes of F

unsigned packetLen = 5;
unsigned dataLen   = 10000;
unsigned objId     = 0;

void setup() {
    WiFi.persistent(false);                  //disable saving wifi config into SDK flash area
    WiFi.mode(WIFI_OFF);                     //disable swAP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
    
    radioControl = new RadioControl(&radio); //init radio control

    radioControl->setupLora(loraTelemetry); //setup lora
    radioControl->setSSDOSender(0xFAFA); // set sender ID for SSDO (some random UINT32)
}

void loop() {
    //Send RAW LORA packet
    //max packet size is 255 bytes (Size is in BYTES)
    //if whant send bigger use SSDO (sendLoraSSDO)
    //or create own protocol
    radioControl->sendLora(packet, packetLen);

    delay(1000);

    // using slow scan digital object protocol
    // no limit in size
    // Avaible types:
    // SSDO_RAW
    // SSDO_ASCII
    // SSDO_JPG
    // SSDO_CHANGE -- special usage
    //radioControl->sendLoraSSDO(longData, dataLen, objId, SSDO_TYPE_ASCII); // send data using ssdo

    // or trasmit with diferent settings
    // this emit SSDO change first to notify reciver
    radioControl->sendLoraSSDO(longData, dataLen, objId, SSDO_TYPE_ASCII, loraHSTelemetry);

    objId++;

    delay(10000);
}
