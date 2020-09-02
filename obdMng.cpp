#include "Arduino.h"
#include "obdMng.h"

uint32_t obdCanIdStd = 0x7E0;
uint32_t obdCanIdExt = 0x18DB33F1;   // BROADCAST ID
uint32_t obdCanIdExtFs = 0x18DA10F1;      // PHYSICAL ID

uint32_t obdCanIdStd_ECU = 0x7E8;
uint32_t obdCanIdExt_ECU = 0x18DAF110;

byte flowStatusFrame[] =  {0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte supPidReq[] =  {0x02, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte noxReq[]=  {0x02, 0x01, 0x083, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void pidRead(uint8_t* sidpr, uint8_t* pid, uint8_t* payloadSize, uint8_t payload[], uint8_t rxBuff[], uint8_t* currPos, bool* finish){
  
  if (*finish == true){

    if(rxBuff[0] == 0x10){
      *finish = false;

      *payloadSize = rxBuff[1] - 6;
      *sidpr = rxBuff[2];
      *pid = rxBuff[3];

      *currPos = 0;
      for (int i = 4; i < 8; i++) {
        payload[*currPos] = rxBuff[i];
        *currPos = *currPos + 1;
      }

    }else if(rxBuff[0] < 0x10){
      *finish = true;

      *payloadSize = rxBuff[0] - 2;
      *sidpr = rxBuff[1];
      *pid = rxBuff[2];

      *currPos = 0;
      for (int i = 3; i < 3 + *payloadSize; i++) {
        payload[*currPos] = rxBuff[i];
        *currPos = *currPos + 1;
      }
      
    }else{
      *finish = true;
      Serial.println(F("ERROR: Please check"));
    }
    
  }else{

    if (rxBuff[0] != (0x20 + ((*currPos + 6) / 8))){
      Serial.println(F("ERROR: Frame number incorrect. Please check"));     
      *finish = true;
    }
    
    for (int i = 1; (*currPos < *payloadSize) && (i < 8); i++) {
        payload[*currPos] = rxBuff[i];
        *currPos = *currPos + 1;
    }

    if (*currPos == *payloadSize){
      *finish = true;
    }
  }
  
}




void pidMng(uint8_t* sidpr, uint8_t* pid, uint8_t* payloadSize, uint8_t payload[]){

//    DEBUG.print("PID: ");
//    DEBUG.println(*pid, HEX);
//    
//    DEBUG.print("SIDPR: ");
//    DEBUG.println(*sidpr, HEX);
//    
//    DEBUG.print("PayloadSize: ");
//    DEBUG.println(*payloadSize);
//    
//    DEBUG.print("Payload: ");
//    for(byte i = 0; i<*payloadSize; i++){
//      if (payload[i]<0x10) DEBUG.print('0');
//      DEBUG.print(payload[i], HEX);
//      DEBUG.print(" ");
//    }
//    DEBUG.println();

  if((*pid == 0x00) || (*pid == 0x20) || (*pid == 0x40) || (*pid == 0x60) || (*pid == 0x80) || (*pid == 0xA0) || (*pid == 0x80)) {
    DEBUG.print(F("Supported PId["));
    if (*pid < 0xF) Serial.print('0');
    DEBUG.print(*pid, HEX);
    DEBUG.print(F("]: "));
    if (payload[0] < 0xF) Serial.print('0');
    DEBUG.print(payload[0], HEX);
    DEBUG.print(' ');
    if (payload[1] < 0xF) Serial.print('0');
    DEBUG.print(payload[1], HEX);
    DEBUG.print(' ');
    if (payload[2] < 0xF) Serial.print('0');
    DEBUG.print(payload[2], HEX);
    DEBUG.print(' ');
    if (payload[3] < 0xF) Serial.print('0');
    DEBUG.print(payload[3], HEX);
    DEBUG.println();
  }

  if(*pid == 0x83){
    DEBUG.print("NOx:");
    if (payload[0] & 0x1){
      uint16_t n1 = ((uint16_t)payload[1] << 8) | payload[2];
      DEBUG.print(" #1: ");
      DEBUG.print(n1);
    }
    if (payload[0] & 0x2){
      uint16_t n2 = ((uint16_t)payload[3] << 8) | payload[4];
      DEBUG.print(" #2: ");
      DEBUG.print(n2);
    }
    DEBUG.println(); 
  }
}
