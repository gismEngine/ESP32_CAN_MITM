#include "Arduino.h"
#include "sockMng.h"

WebSocketsServer webSocket = WebSocketsServer(81);

bool newClient = false;
bool newRemoteData = false;

enum conversionMode convModeNox = CONV_DIS;
enum conversionMode convModeO2 = CONV_DIS;

uint32_t nox_fix_bypass = 0x00;
uint32_t o2_fix_bypass = 0x00;

float nox_prop = 1.0;
float o2_prop = 1.0;

void websocket_begin(){
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void websocket_manage() {
    webSocket.loop();
}

void broadcastJsonById(char* id, char* value){ 
    DynamicJsonDocument doc(1024);
    String pl;
    doc["ID"] = id;
    doc["data"] = value;
    serializeJson(doc, pl);
    webSocket.broadcastTXT(pl);
}

void broadcastJsonById(char* id, uint32_t v){ 
    DynamicJsonDocument doc(1024);
    String pl;
    doc["ID"] = id;
    doc["data"] = v;
    serializeJson(doc, pl);
    webSocket.broadcastTXT(pl);
}

void broadcastJsonById(char* id, double v){ 
    DynamicJsonDocument doc(1024);
    String pl;
    doc["ID"] = id;
    doc["data"] = v;
    serializeJson(doc, pl);
    webSocket.broadcastTXT(pl);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                newClient = true;
                
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

                // send message to client
                webSocket.sendTXT(num, "Hello Client :)");
            }
            break;
        case WStype_TEXT:
            {
              Serial.printf("[%u] get Text: %s\n", num, payload);
  
              DynamicJsonDocument received_doc(1024);
              auto error1 = deserializeJson(received_doc, payload);
              // serializeJson(received_doc, Serial);
              // Serial.println();
  
              if (error1) {
                Serial.print(F("Error1: deserializeJson() failed with code "));
                Serial.println(error1.c_str());
                return;
              }
  
              String r_id = received_doc["ID"];
              String r_data = received_doc["data"];
  
              if (r_id.equalsIgnoreCase("cmd")){
                if (r_data.equalsIgnoreCase("ModeChnNox")) {
                  
                  switch (convModeNox) {
                    case CONV_DIS:
                      convModeNox = CONV_FIX;
                      break;
                    case CONV_FIX:
                      convModeNox = CONV_PROP;
                      break;
                    case CONV_PROP:
                      convModeNox = CONV_MAP;
                      break;
                    case CONV_MAP:
                      convModeNox = CONV_DIS;
                      break;  
                    default:
                      convModeNox = CONV_DIS;
                      break;
                  }
                }else if (r_data.equalsIgnoreCase("ModeChnO2")){
                  
                  switch (convModeO2) {
                    case CONV_DIS:
                      convModeO2 = CONV_FIX;
                      break;
                    case CONV_FIX:
                      convModeO2 = CONV_PROP;
                      break;
                    case CONV_PROP:
                      convModeO2 = CONV_MAP;
                      break;
                    case CONV_MAP:
                      convModeO2 = CONV_DIS;
                      break;  
                    default:
                      convModeO2 = CONV_DIS;
                      break;
                  }
                }else if (r_id.equalsIgnoreCase("nox_fix")){
                  nox_fix_bypass = atol(received_doc["data"]);
                  
                  Serial.print(F("New NOx Fix: "));
                  Serial.print(nox_fix_bypass);
                  Serial.print(F(" - 0x"));
                  Serial.println(nox_fix_bypass, HEX);
                
                }else if(r_id.equalsIgnoreCase("nox_prop")){
                  nox_prop = (float)atof(received_doc["data"]);

                  Serial.print("New NOx Prop: ");
                  Serial.println(nox_prop);
                
                }else if (r_id.equalsIgnoreCase("o2_fix")){
                  o2_fix_bypass = atol(received_doc["data"]);

                  Serial.print("New O2 Fix: ");
                  Serial.print(o2_fix_bypass);
                  Serial.print(F(" - 0x"));
                  Serial.println(o2_fix_bypass, HEX);
                
                }else if (r_id.equalsIgnoreCase("o2_prop")){
                  o2_prop = (float)atof(received_doc["data"]);
  
                  Serial.print("New O2 Prop: ");
                  Serial.println(o2_prop);
                }

              newRemoteData = true;
              }
            }
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
    }
}

void hexdump(const void *mem, uint32_t len, uint8_t cols) {
  const uint8_t* src = (const uint8_t*) mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for(uint32_t i = 0; i < len; i++) {
    if(i % cols == 0) {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}

bool isNewClient(void){
  if(newClient == true){
    newClient = false;
    return true;
  }
  return false;
}

bool isNewRemoteData(void){
  if(newRemoteData == true){
    newRemoteData = false;
    return true;
  }
  return false;
}


enum conversionMode getConversionModeNox(void){
  return convModeNox;
}

enum conversionMode getConversionModeO2(void){
  return convModeO2;
}

uint32_t getNoxFixBypass(void){
  return nox_fix_bypass;
}

uint32_t getO2FixBypass(void){
  return o2_fix_bypass;
}

float getNoxProp(void){
  return nox_prop;
}

float getO2Prop(void){
  return o2_prop;
}
