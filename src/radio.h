/**
 * espSat project simple esp base satellite
 * File with header of radio manager
 * @author Lukas Plevac <lukas@plevac.eu>
 */

#pragma once

#include <RadioLib.h>
#include "./debug.h"
#include <ssdo.h>

#define RTTY_IDLE_TIME 10

// define version of module
#define RADIO_VERSION 1

// Struct to hold LoRA settings
typedef struct {
  float Frequency;
  float Bandwidth;
  uint8_t SpreadFactor;
  uint8_t CodeRate;
  uint8_t SyncWord;
  uint8_t Power;
  uint8_t CurrentLimit;
  uint16_t PreambleLength;
  uint8_t Gain;
} LoraSettings_t;

// Struct to hold FSK settings
typedef struct {
  float Frequency;
  float BitRate; 
  float FreqDev;
  float RXBandwidth;
  int8_t  Power;             // in dbM range 2 - 17
  uint16_t PreambleLength;
  bool  EnableOOK;
  float dataShaping;
} FSKSettings_t;

// Struct to hold RTTY settings
typedef struct {
  float Frequency;    // Base frequency
  uint32_t Shift;     // RTTY shift
  uint16_t Baud;      // Baud rate
  uint8_t Encoding;   // Encoding (ASCII = 7 bits)
  uint8_t StopBits;   // Number of stopbits 
} RTTYSettings_t;

typedef struct {
  float Frequency;    // Base frequency
  float Correction;   // correction to slow down or speed up clock
  SSTVMode_t Mode;    // SSTV mode
} SSTVSettings_t;

class RadioControl {
    public:
        /**
         * radioControl conscrutor setuped basic HW layer for access radio
         * for more features like rtty, FSK, lora and sstv need to setup this features before use
          * @param RadioSettings struct with radio module settings
         */
        RadioControl(RADIOHW *radio);

        /**
         * Setup RTTY radio feature
         * @param RTTYSettings settings to set
         * @return bool
         */
        bool setupRTTY(RTTYSettings_t RTTYSettings, RTTYClient *rtty);

        /**
         * Setup FSK radio feature
         * @param FSKSettings settings to set
         * @return bool
        */
        bool setupFSK(FSKSettings_t FSKSettings);

        /**
         * Setup SSTV protocol
         * @param SSTVSetting sstv settings
         * @param sstv sstv client to set
         * @retun bool
         */
        bool setupSSTV(SSTVSettings_t SSTVSettings, SSTVClient *sstv);

        /**
         * Setup LORA radio feature
         * @param LoRaSettings settings to set
         * @return bool
         */
        bool setupLora(LoraSettings_t LoraSettings, bool is_default = true);

        /**
         * send RTTY telemetry message
         * @param message TxLine to send
         * @return bool
         */
        bool sendRTTY(String message);

        /**
         * send LORA message
         * @param message data to send
         * @param size size of data in bytes
         * @return bool
         */
        bool sendLora(uint8_t* message, unsigned size);

        void setSSDOSender(uint32_t senderId);
        
        bool sendLoraSSDO(uint8_t* obj, unsigned size, uint32_t objectId, uint8_t objType = SSDO_TYPE_RAW);

        bool sendLoraSSDO(uint8_t* obj, unsigned size, uint32_t objectId, uint8_t objType, LoraSettings_t newLoRaSettings, unsigned resend = 1);

        /**
         * send LORA string message
         * @param message string to send
         */
        //bool sendLora(String message);

        /**
         * send SSTV image
         * @param image pointer to image in RGB565 
         * @return bool
         */
        bool sendSSTV(uint16_t *image);

        bool sendSSTVGS(uint8_t *image);
        
        void resetLora();

        void setLoraReceiveHandler(void (*handler)(uint8_t*));

        void setLoraSSDOPacketHandler(void (*handler)(uint8_t*, ssdoHeader_t*));

        void setLoraSSDOObjectHandler(void (*handler)(uint8_t*, ssdoHeader_t*));

        ssdoChange_t   changeEncode(LoraSettings_t newSettings);
        LoraSettings_t changeDecode(ssdoChange_t* newSettings, LoraSettings_t* defaults);

        void processRecvBuff();

        RADIOHW    *radio;

        void (*loraHandler)(uint8_t*);
        void (*ssdoPacketHandler)(uint8_t*, ssdoHeader_t*);
        void (*ssdoObjectHandler)(uint8_t*, ssdoHeader_t*);

        hw_timer_t* ssdoResetTimer;

        LoraSettings_t LoRaSettings;

    private:
        
        
        RTTYClient *rtty;
        SSTVClient *sstv;
        
        uint32_t SSDOSenderId;

        bool fskReady;
        bool rttyReady;
        bool loraReady;
        bool sstvReady;

};
