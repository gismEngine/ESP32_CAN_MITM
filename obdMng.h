#ifndef obdMng_h
#define obdMng_h

  #include <Arduino.h>
  #define DEBUG Serial

  extern uint32_t obdCanIdStd;
  extern uint32_t obdCanIdExt;
  extern uint32_t obdCanIdExtFs;

  extern uint32_t obdCanIdStd_ECU;  

  extern byte flowStatusFrame[8];

  extern byte supPidReq[8];
  extern byte noxReq[8];

  void pidRead(uint8_t* sidpr, uint8_t* pid, uint8_t* payloadSize, uint8_t payload[], uint8_t rxBuff[], uint8_t* currPos, bool* finish);
  void pidMng(uint8_t* sidpr, uint8_t* pid, uint8_t* payloadSize, uint8_t payload[]);
  
#endif
