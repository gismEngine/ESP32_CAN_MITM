#ifndef sockMng_h
#define sockMng_h

  #include <Arduino.h>
  #include <WebSocketsServer.h>

  #include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson

  enum conversionMode {
    CONV_DIS,          // Conversion disable
    CONV_FIX,          // Fix conversion
    CONV_PROP,         // we want the led to be on for interval
    CONV_MAP           // we want the led to be off for interval
  };

  void websocket_begin();
  void websocket_manage();
  void broadcastJsonById(char* id, uint32_t v);
  void broadcastJsonById(char* id, double v);
  void broadcastJsonById(char* id, char* value);

  bool isNewClient(void);
  bool isNewRemoteData(void);
  enum conversionMode getConversionModeNox(void);
  enum conversionMode getConversionModeO2(void);

  uint32_t getNoxFixBypass(void);
  uint32_t getO2FixBypass(void);
  float getNoxProp(void);
  float getO2Prop(void);
  
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
  void hexdump(const void *mem, uint32_t len, uint8_t cols = 16);
  
#endif
