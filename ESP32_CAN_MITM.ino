// ESP32 CAN MITM

// ESP32 with 2 CAN Channel input
// NOT use of ESP32 internal CAN-bus HW.
// 2 MCP2515 Board used instead
// ESP32 prints by UART (DEBUG) CAN traffic as ASC file format
// When bridge flag is set ('b' command on UART):
// CAN traffic from CAN1 is send to CAN2 and vice versa

// Default SPI pins on ESP32
// MOSI:  D23
// MISO:  D19
// SCK:   D18

// MCP2515 Board connected to VIN (5v)
// ESP32 is NOT officialy 5V input tolerant
// however in practive, IT IS (apparently)
// MOSI and MISO can be connected dirrectly from ESP32 and MCP2515 Board :)

#include <mcp_can.h>
#include <SPI.h>

#define CAN1_INT 21
#define CAN2_INT 15

#define CAN1_CS 5
#define CAN2_CS 4

const char* PROGMEM FIRMWARE_NAME = "ESP_CAN_tool";
const char* PROGMEM FIRMWARE_VER = "0.1";

byte testData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
uint32_t testFrameTime = 1000;

#define DEBUG Serial

bool canBridge = false;

MCP_CAN CAN1(CAN1_CS);     // Set CS to pin 10
MCP_CAN CAN2(CAN2_CS);     // Set CS to pin 10

INT32U rxId;
uint8_t len = 0;
uint8_t rxBuf[8];
char msgString[128];                        // Array to store serial string

uint32_t frameTime = 0;

void setup()
{
  DEBUG.begin(115200);
  
  DEBUG.print(F("\n\n\n"));                         //  Print firmware name and version const string
  DEBUG.print(FIRMWARE_NAME);
  DEBUG.print(F(" - "));
  DEBUG.println(FIRMWARE_VER);

  DEBUG.print(F("CPU@ "));      
  DEBUG.print(getCpuFrequencyMhz());    // In MHz
  DEBUG.println(F(" MHz"));

  DEBUG.print(F("Xtal@ "));      
  DEBUG.print(getXtalFrequencyMhz());    // In MHz
  DEBUG.println(F(" MHz"));

  DEBUG.print(F("APB@ "));      
  DEBUG.print(getApbFrequency());        // In Hz
  DEBUG.println(F(" Hz"));

  EspClass e;
  
  DEBUG.print(F("SDK v: "));
  DEBUG.println(e.getSdkVersion());

  DEBUG.print(F("Flash size: "));
  DEBUG.println(e.getFlashChipSize());
 
  DEBUG.print(F("Flash Speed: "));
  DEBUG.println(e.getFlashChipSpeed());
 
  DEBUG.print(F("Chip Mode: "));
  DEBUG.println(e.getFlashChipMode());

  DEBUG.print(F("Sketch size: "));
  DEBUG.println(e.getSketchSize());

  DEBUG.print(F("Sketch MD5: "));
  DEBUG.println(e.getSketchMD5());

  Serial.println(F("CAN1 Setup:"));
  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN1.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK){
    Serial.println(F("MCP2515 Initialized Successfully!"));
  }else{
    Serial.println(F("Error Initializing MCP2515..."));
  }
  CAN1.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  
  pinMode(CAN1_INT, INPUT);
  
  Serial.println(F("CAN2:"));
  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN2.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK){
    Serial.println("MCP2515 Initialized Successfully!");
  }else{
    Serial.println("Error Initializing MCP2515...");
  }
  CAN2.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  
  pinMode(CAN2_INT, INPUT);

  frameTime = millis(); 
}

void loop()
{
  canTxMng();
  canRxMng();

  debugCmdMng();
}


void canRxMng(void){

  // If CAN*_INT pin is low, read receive buffer
  
  if(!digitalRead(CAN1_INT)){
    canChRxMng(&CAN1);
  }
  
  if(!digitalRead(CAN2_INT)){
    canChRxMng(&CAN2);
  }
}


void canChRxMng(MCP_CAN* canCh){

    // Check CAN channel:
    uint8_t chNum = 0;
    if (canCh == &CAN1){
      chNum = 1;      
    }else if(canCh == &CAN2){
      chNum = 2;
    }

    // Read data:
    // rxId = frame ID
    // len = CAN frame length (DLC)
    // buf = frame payload: data byte(s)
    canCh->readMsgBuf(&rxId, &len, rxBuf);

    // Print timestamp:
    sprintf(msgString, "%f\t", (float)millis()/1000);
    DEBUG.print(msgString);
    
    // Determine if ID is standard (11 bits) or extended (29 bits)
    uint8_t ext = 0;
    if((rxId & 0x80000000) == 0x80000000){ 
      // Extended CAN ID:
      ext = 1; 
      sprintf(msgString, "%d\tx%.8lX\tRx\t", chNum, (rxId & 0x1FFFFFFF));
    }else{
      // Standard CAN ID:
      ext = 0;
      sprintf(msgString, "%d\t%.3lX\tRx\t", chNum, rxId);
    }
    DEBUG.print(msgString);

    // Check RTR bit and determine if message is a remote request frame.
    // datatype: 
    // r == remote_frame
    // d == datatype
    if((rxId & 0x40000000) == 0x40000000){
      sprintf(msgString, "r\t%d\t", len);
      DEBUG.print(msgString);
    } else {
      
      sprintf(msgString, "d\t%d\t", len);
      DEBUG.print(msgString);
      
      for(byte i = 0; i<len; i++){
        if (rxBuf[i]<0x10) DEBUG.print('0');
        DEBUG.print(rxBuf[i], HEX);
        DEBUG.print(" ");
      }
    }        
    DEBUG.println();

    // if CAN bridge is active. Send all CAN Rx frames to other CAN channel
    if(canBridge){
      if (chNum == 1){
        sendFrame(&CAN2, rxId, ext, len, rxBuf);
      }else if (chNum == 2){
        sendFrame(&CAN1, rxId, ext, len, rxBuf);
      }
    }    
}

void canTxMng(void){

  if (millis() >= frameTime + testFrameTime){
    sendTestFrame();
    frameTime = millis(); 
  }
  
}

void sendTestFrame(void){
  // send data:  ID = 0x555, Standard CAN Frame, Data length = 8 bytes, 'testData' = array of data bytes to send
  sendFrame(&CAN1, 0x555, 0, 8, testData);
}

void sendFrame(MCP_CAN* canCh, uint32_t id, uint8_t ext, uint8_t len, uint8_t *payload){

    byte sndStat = canCh->sendMsgBuf(id, ext, len, payload);
    if(sndStat == CAN_OK){
      
      // Check CAN channel:
      uint8_t chNum = 0;
      if (canCh == &CAN1){
        chNum = 1;      
      }else if(canCh == &CAN2){
        chNum = 2;
      }
  
      // Print timestamp:
      sprintf(msgString, "%f\t", (float)millis()/1000);
      DEBUG.print(msgString);
      
      // Determine if ID is standard (11 bits) or extended (29 bits)
      if((rxId & 0x80000000) == 0x80000000){ 
        // Extended CAN ID:    
        sprintf(msgString, "%d\tx%.8lX\tTx\t", chNum, (id & 0x1FFFFFFF));
      }else{
        // Standard CAN ID:
        sprintf(msgString, "%d\t%.3lX\tTx\t", chNum, id);
      }
      DEBUG.print(msgString);
  
      // Check RTR bit and determine if message is a remote request frame.
      // datatype: 
      // r == remote_frame
      // d == datatype
      if((id & 0x40000000) == 0x40000000){
        sprintf(msgString, "r\t%d\t", len);
        DEBUG.print(msgString);
      } else {
        
        sprintf(msgString, "d\t%d\t", len);
        DEBUG.print(msgString);
        
        for(byte i = 0; i<len; i++){
          if (payload[i]<0x10) DEBUG.print('0');
          DEBUG.print(payload[i], HEX);
          DEBUG.print(" ");
        }
      }        
      DEBUG.println();

    } else {
      Serial.println(F("Error Sending Message..."));
    }
}


void debugCmdMng(void){
  if (DEBUG.available()){
     char c = DEBUG.read();
     if (c == 'b'){
        DEBUG.print("CAN Bridge: ");
        canBridge = !canBridge;
        DEBUG.println(canBridge);
     }
  }
}
