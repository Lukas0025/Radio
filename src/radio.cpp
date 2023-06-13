/**
 * espSat project simple esp base satellite
 * File with implementation of radio manager
 * @author Lukas Plevac <lukas@plevac.eu>
 */
#include "radio.h"

#define RECV_BUFF_SIZE 10

uint8_t RadioRecvBuffer[SSDO_PACKET_SIZE * RECV_BUFF_SIZE];
uint8_t ssdoBodyBuff[SSDO_PACKET_SIZE];
ssdoHeader_t ssdoHeaderBuff;

unsigned     recvBuffWritePos = 0;
unsigned     recvBuffReadPos  = 0;

bool         rebootLora       = false;

RadioControl* activeRC;

/**
 * Interupts handlers
 */

void handleLoraReceive() {
  activeRC->radio->readData(RadioRecvBuffer + SSDO_PACKET_SIZE * recvBuffWritePos, 0);
  recvBuffWritePos = (recvBuffWritePos + 1) % RECV_BUFF_SIZE;
  timerWrite(activeRC->ssdoResetTimer, 0);
}

void handleSSDOPacket(uint8_t* packet) {
  if (!SSDO::decodePacket(packet, ssdoBodyBuff, &ssdoHeaderBuff)) return;

  if (ssdoHeaderBuff.objType == SSDO_TYPE_CHANGE) {

    DEBUG_PRINT("SSDO change req");

    // SSDO change req
    LoraSettings_t newSettings   = activeRC->changeDecode((ssdoChange_t*) ssdoBodyBuff, &(activeRC->LoRaSettings));
    activeRC->setupLora(newSettings, false);
    activeRC->radio->startReceive();

    // start reset timer
    timerWrite(activeRC->ssdoResetTimer, 0);
    timerAlarmEnable(activeRC->ssdoResetTimer);
  } else {
    activeRC->ssdoPacketHandler(ssdoBodyBuff, &ssdoHeaderBuff);
  }
}

void handleSSDOObject(uint8_t* body, ssdoHeader_t* header) {
  //todo
}

void RadioControl::processRecvBuff() {
  if (rebootLora) {
    rebootLora = false;
    this->resetLora();
    this->radio->startReceive();
  }

  if (recvBuffWritePos == recvBuffReadPos) return;

  this->loraHandler(RadioRecvBuffer + SSDO_PACKET_SIZE * recvBuffReadPos);

  recvBuffReadPos = (recvBuffReadPos + 1) % RECV_BUFF_SIZE;
}

void resetLoraHandler() {
  rebootLora = true;
}

/**
 * End of interupts handlers
 */

RadioControl::RadioControl(RADIOHW *radio) {
  this->radio     = radio;
                        
  this->fskReady  = false;
  this->rttyReady = false;
  this->loraReady = false;
  this->sstvReady = false;

  this->ssdoResetTimer = timerBegin(0, 80, true);                 	  // timer 0, prescalar: 80, UP counting
  timerAttachInterrupt(this->ssdoResetTimer, resetLoraHandler, true); 	// Attach interrupt
  timerAlarmWrite(this->ssdoResetTimer, 1000000 * 30, true);     		  // Match value= 1000000 for 1 sec. delay.
}

bool RadioControl::setupRTTY(RTTYSettings_t RTTYSettings, RTTYClient *rtty) {
  
  DEBUG_PRINT("Stuping RTTY");

  if (!this->fskReady) {
    ERROR_PRINT("fsk not setuped");
    return false;
  }

  this->rtty = rtty;
  
  int16_t state = this->rtty->begin(
    RTTYSettings.Frequency,
    RTTYSettings.Shift,
    RTTYSettings.Baud,
    RTTYSettings.Encoding,
    RTTYSettings.StopBits
  );
                     
  if(state != RADIOLIB_ERR_NONE) {
    ERROR_PRINT("Fail to setup RTTY. Error code: ", state);
    return false;
  }

  DEBUG_PRINT("RTTY setuped");
  
  this->rttyReady = true;
  return true;
}

bool RadioControl::setupFSK(FSKSettings_t FSKSettings) {

  DEBUG_PRINT("Setuping FSK");
  delay(1000);
  DEBUG_PRINT("Setuping FSK");
 
  int16_t state = this->radio->beginFSK(
    FSKSettings.Frequency,
    FSKSettings.BitRate,
    FSKSettings.FreqDev,
    FSKSettings.RXBandwidth,
    FSKSettings.Power,
    FSKSettings.PreambleLength,
    FSKSettings.EnableOOK
  );


  if(state != RADIOLIB_ERR_NONE) {
    ERROR_PRINT("Fail to setup FSK. Error code: ", state);
    return false;
  }

  this->fskReady  = true;
  this->loraReady = false;
  DEBUG_PRINT("FSK setuped");
  
  return true;
}

bool RadioControl::setupSSTV(SSTVSettings_t ssvtsettings, SSTVClient *sstv) {

  DEBUG_PRINT("Setuping SSTV");
  
  int state = sstv->begin(ssvtsettings.Frequency, ssvtsettings.Mode);

  if(state != RADIOLIB_ERR_NONE) {
    ERROR_PRINT("Fail to setup SSTV. Error code: ", state);
    return false;
  }

  state = sstv->setCorrection(ssvtsettings.Correction);
  
  if(state != RADIOLIB_ERR_NONE) {
    ERROR_PRINT("Fail to set SSTV Correction. Error code: ", state);
    return false;
  }

  this->sstvReady  = true;
  this->loraReady  = false;
  this->fskReady   = false;
  this->rttyReady  = false;

  this->sstv = sstv;

  DEBUG_PRINT("SSTV stuped");

  return true;
}

bool RadioControl::setupLora(LoraSettings_t LoRaSettings, bool is_default) {
  
  DEBUG_PRINT("Setuping LORA");

  if (is_default) this->LoRaSettings = LoRaSettings;

  int16_t state = this->radio->begin(
    LoRaSettings.Frequency,
    LoRaSettings.Bandwidth,
    LoRaSettings.SpreadFactor,
    LoRaSettings.CodeRate,
    LoRaSettings.SyncWord,
    LoRaSettings.Power,
    LoRaSettings.PreambleLength, 
    LoRaSettings.Gain
  );
  
  this->radio->setCRC(true);
  
  if(state != RADIOLIB_ERR_NONE) {
    ERROR_PRINT("Fail to setup LORA. Error code: ", state);
    return false;
  }

  this->loraReady  = true;
  this->fskReady   = false;
  this->rttyReady  = false;
  this->sstvReady  = false;
  DEBUG_PRINT("LORA setuped");
  
  return true;
}

bool RadioControl::sendRTTY(String message) {

  if (!this->rttyReady) {
    ERROR_PRINT("rtty not setuped");
    return false;
  }
  
  DEBUG_PRINT("rtty IDLE");
  this->rtty->idle();     

  delay(RTTY_IDLE_TIME); 

  DEBUG_PRINT("rtty sending ", message); 
  this->rtty->println(message);
  
  // turn off transmitter
  this->radio->standby(); 

  return true;
}

bool RadioControl::sendLora(uint8_t* message, unsigned size) {
  if (!this->loraReady) {
    ERROR_PRINT("ERROR lora not setuped");
    return false;
  }
  
  this->radio->transmit(message, size);

  // turn off transmitter
  //this->radio->standby(); 
  
  return true;
}

ssdoChange_t RadioControl::changeEncode(LoraSettings_t newSettings) {
  ssdoChange_t newSSDOSettings;

	newSSDOSettings.Frequency    = newSettings.Frequency;
	newSSDOSettings.Bandwidth    = newSettings.Bandwidth;
	newSSDOSettings.SpreadFactor = newSettings.SpreadFactor;
	newSSDOSettings.CodeRate     = newSettings.CodeRate;
	newSSDOSettings.SyncWord     = newSettings.SyncWord;

	return newSSDOSettings;
}

LoraSettings_t RadioControl::changeDecode(ssdoChange_t *newSettings, LoraSettings_t *defaults) {
  LoraSettings_t newLORASettings;

  memcpy(&newLORASettings, defaults, sizeof(LoraSettings_t));

	newLORASettings.Frequency    = newSettings->Frequency;
	newLORASettings.Bandwidth    = newSettings->Bandwidth;
	newLORASettings.SpreadFactor = newSettings->SpreadFactor;
	newLORASettings.CodeRate     = newSettings->CodeRate;
	newLORASettings.SyncWord     = newSettings->SyncWord;

	return newLORASettings;
}

void RadioControl::setSSDOSender(uint32_t senderId) {
  this->SSDOSenderId = senderId;
}

void RadioControl::resetLora() {
  this->setupLora(this->LoRaSettings, false);
  timerAlarmDisable(this->ssdoResetTimer);
}

void RadioControl::setLoraReceiveHandler(void (*handler)(uint8_t*)) {
  this->radio->setDio0Action(handleLoraReceive, RISING);
  this->loraHandler = handler;

  this->radio->startReceive();
  
  activeRC = this;
}

void RadioControl::setLoraSSDOPacketHandler(void (*handler)(uint8_t*, ssdoHeader_t*)) {
  this->setLoraReceiveHandler(handleSSDOPacket);
  this->ssdoPacketHandler = handler;
}

void RadioControl::setLoraSSDOObjectHandler(void (*handler)(uint8_t*, ssdoHeader_t*)) {
  this->setLoraSSDOPacketHandler(handleSSDOObject);
  this->ssdoObjectHandler = handler;
}

bool RadioControl::sendLoraSSDO(uint8_t* obj, unsigned size, uint32_t objectId, uint8_t objType) {
  
  SSDO ssdoProtocol = SSDO(this->SSDOSenderId, objectId, objType);

  uint8_t packet[SSDO_PACKET_SIZE];

	for (unsigned i = 0; i < ssdoProtocol.packetsCount(size); i++) { 
		unsigned packetLen = ssdoProtocol.setPacket(obj, i, packet, size);
		this->sendLora(packet, packetLen);
	}

  return true;
}

bool RadioControl::sendLoraSSDO(uint8_t* obj, unsigned size, uint32_t objectId, uint8_t objType, LoraSettings_t newLoRaSettings, unsigned resend) {
  
  if (memcmp(&newLoRaSettings, &(this->LoRaSettings), sizeof(LoraSettings_t)) != 0) {

    SSDO ssdoProtocol    = SSDO(this->SSDOSenderId, objectId, SSDO_TYPE_CHANGE);

    auto newSSDOSettings = this->changeEncode(LoRaSettings);

    uint8_t packet[SSDO_PACKET_SIZE];

    for (unsigned retry = 0; retry < resend; retry++)
	  for (unsigned i = 0; i < ssdoProtocol.packetsCount(sizeof(ssdoChange_t)); i++) { 
		  unsigned packetLen = ssdoProtocol.setPacket((uint8_t*) &newSSDOSettings, i, packet, sizeof(ssdoChange_t));
		  this->sendLora(packet, packetLen);
	  }
  }

  this->setupLora(newLoRaSettings, false);

  delay(1000);

  bool res = this->sendLoraSSDO(obj, size, objectId, objType);

  this->setupLora(this->LoRaSettings, false);

  return res;
}

bool RadioControl::sendSSTV(uint16_t *image) {

  if (!this->sstvReady) {
    ERROR_PRINT("ERROR sstv not setuped");
    return false;
  }

  DEBUG_PRINT("sstv send sync tone");
  this->sstv->idle();
  delay(10000);
  
  DEBUG_PRINT("sstv set headers");
  this->sstv->sendHeader();  

  for(uint8_t i = 0; i < 240; i++) {
    uint32_t line[320];

    //parse line
    for (uint16_t j = 0; j < 320; j++) {
      uint8_t r = (image[i * 320 + j] >> 11) & 0x1F;  
      uint8_t g = (image[i * 320 + j] >> 5)  & 0x3F;
      uint8_t b =  image[i * 320 + j]        & 0x1F;

      r = (r * 255 + 15) / 31;
      b = (b * 255 + 15) / 31;
      g = (g * 255 + 31) / 63;
      
      line[j] = (r << 16) | (g << 8) | b;
    }
    
    DEBUG_PRINT("sstv sending line ", i); 
    this->sstv->sendLine(line);
  }

  // turn off transmitter
  this->radio->standby(); 

  return true;
}

bool RadioControl::sendSSTVGS(uint8_t *image) {

  if (!this->sstvReady) {
    ERROR_PRINT("ERROR sstv not setuped");
    return false;
  }

  DEBUG_PRINT("sstv send sync tone");
  this->sstv->idle();
  delay(10000);
  
  DEBUG_PRINT("sstv set headers");
  this->sstv->sendHeader();  

  for(uint8_t i = 0; i < 240; i++) {
    uint32_t line[320];

    //parse line
    for (uint16_t j = 0; j < 320; j++) {
      line[j] = (image[j] << 16) | (image[j] << 8) | image[j];
    }
    
    DEBUG_PRINT("sstv sending line ", i); 
    this->sstv->sendLine(line);
  }

  // turn off transmitter
  this->radio->standby(); 

  return true;
}
