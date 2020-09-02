
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

#include "obdMng.h"

#include "webMng.h"
#include "sockMng.h"

#define CAN1_INT 15
#define CAN2_INT 21

#define CAN1_CS 4
#define CAN2_CS 5

#define NOX_REQ_TIME 200

const char* PROGMEM FIRMWARE_NAME = "ESP_DUAL_CAN";
const char* PROGMEM FIRMWARE_VER = "0.1";

byte testData[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

// For test frame
uint32_t frameTime = 0;
uint32_t testFrameTime = 0;

#define DEBUG Serial

bool canBridge = false;
bool canPrint = false;
bool can1Print = false;
bool can2Print = false;

bool printTempReached = false;

bool finish = true;
bool waitingObdReply = false;

bool askSupportedPid = false;
uint8_t askSupportedPidCount = 0;

bool askNoxObd = false;
uint32_t askNoxTime = 0;

MCP_CAN CAN1(CAN1_CS);     // Set CS to pin 10
MCP_CAN CAN2(CAN2_CS);     // Set CS to pin 10

INT32U rxId;
uint8_t len = 0;
uint8_t rxBuf[8];
char msgString[128];                        // Array to store serial string

enum conversionMode convModeNox_main;
enum conversionMode convModeO2_main;
uint32_t nox_fix_bypass_main;
uint32_t o2_fix_bypass_main;
float nox_prop_main;
float o2_prop_main;

void setup() {
  DEBUG.begin(115200);
  printEspInfo();

  // Init wifi + web interface:
  #if defined (WIFI_AP_MODE) || defined (WIFI_STA_MODE)
    webMng.begin(FIRMWARE_NAME, FIRMWARE_VER);        // Init Wifi-AP + Webserver + WebSocket + DNSserver
    websocket_begin();
  #else
    DEBUG.println(F("WARNING: No wifi mode defined --> Wifi disabled"));
  #endif
  
  // Init CAN (MCP2515) interfaces:
  DEBUG.println(F("CAN-Channel 1 Setup:"));

  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN1.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    DEBUG.println(F("MCP2515 Init: OK!"));
  } else {
    DEBUG.println(F("/!\ ERROR Initializing MCP2515..."));
  }
  CAN1.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  pinMode(CAN1_INT, INPUT);

  Serial.println(F("CAN-Channel 2 Setup:"));

  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN2.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    DEBUG.println(F("MCP2515 Init: OK!"));
  } else {
    DEBUG.println(F("/!\ ERROR Initializing MCP2515..."));
  }
  CAN2.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  pinMode(CAN2_INT, INPUT);

  frameTime = millis();
}

void loop() {

  canTxMng();
  canRxMng();

  #if defined (WIFI_AP_MODE) || defined (WIFI_STA_MODE)
    webMng.manage();         // Do wifi stuff
    websocket_manage();      // Do websocket stuff
  #endif

  debugCmdMng();

  if (isNewRemoteData() || isNewClient()) {
    broadcastUiData();
  }
}

void canRxMng(void) {

  // If CAN*_INT pin is low, read receive buffer

  if (!digitalRead(CAN1_INT)) {
    canChRxMng(&CAN1);
  }

  if (!digitalRead(CAN2_INT)) {
    canChRxMng(&CAN2);
  }
}


void canTxMng(void) {

  if (testFrameTime != 0) {
    if (millis() >= frameTime + testFrameTime) {
      sendTestFrame();
      frameTime = millis();
    }
  }

  bool sendOk = false;

  if (askSupportedPid) {
    //Serial.println("askSupportedPid: ");
    if (!waitingObdReply) {
      Serial.print("waitingObdReply: ");
      Serial.println(waitingObdReply);
      supPidReq[2] = askSupportedPidCount;
      sendOk = sendFrame(&CAN1, obdCanIdExt, true, 8, supPidReq);
      if (sendOk) {
        Serial.print("SEND OK");
        if (askSupportedPidCount <= 0xC0) {   // To generate serie: 0x00, 0x20, 0x40, 0x80, 0xA0, 0xC0
          askSupportedPidCount = askSupportedPidCount + 0x20;
        } else {
          askSupportedPidCount = 0x00;
          askSupportedPid = false;
        }
        waitingObdReply = true;
      }
    }
  }

  if (askNoxObd) {
    if (!waitingObdReply) {
      if (millis() >= askNoxTime + NOX_REQ_TIME) {
        bool sendOk = sendFrame(&CAN1, obdCanIdStd, false, 8, noxReq);
        if (sendOk) {
          waitingObdReply = true;
          askNoxTime = millis();  // if not posible to send frame (arbitration, electrical, ...) repeat again next loop
        }
      }
    }
  }
}


void canChRxMng(MCP_CAN* canCh) {

  uint8_t sidpr = 0;
  uint8_t pId = 0;
  uint8_t payloadSize = 0;
  uint8_t rxObdBuf[20];
  uint8_t currPos = 0;

  // Check CAN channel:
  uint8_t chNum = 0;
  if (canCh == &CAN1) {
    chNum = 1;
  } else if (canCh == &CAN2) {
    chNum = 2;
  }

  // Read data:
  // rxId = frame ID
  // len = CAN frame length (DLC)
  // buf = frame payload: data byte(s)
  canCh->readMsgBuf(&rxId, &len, rxBuf);

  // Determine if ID is standard (11 bits) or extended (29 bits)
  uint8_t extId = 0;
  if ((rxId & 0x80000000) == 0x80000000) {
    extId = 1;
  }

  if (canPrint || (can1Print && (chNum == 1)) || (can2Print && (chNum == 2))) {
    printAsc(chNum, extId);
  }

  if ((rxId == 0x264) || (rxId == 0x284)) {
    if (convModeNox_main == CONV_FIX) {
      uint8_t nox_msb_fix = nox_fix_bypass_main >> 8;
      uint8_t nox_lsb_fix = (uint8_t)(nox_fix_bypass_main & 0xFF);

      rxBuf[2] = nox_msb_fix;
      rxBuf[3] = nox_lsb_fix;

    } else if (convModeNox_main == CONV_PROP) {

      uint16_t nox = ((uint16_t)rxBuf[2] << 8) | rxBuf[3];

      nox = nox * nox_prop_main;

      uint8_t nox_msb_prop = nox >> 8;
      uint8_t nox_lsb_prop = (uint8_t)(nox & 0xFF);

      rxBuf[1] = nox_msb_prop;
      rxBuf[2] = nox_lsb_prop;

    }//else if(convMode == CONV_MAP){
    //
    //}

    if (convModeO2_main == CONV_FIX) {

      uint8_t o2_msb_fix = o2_fix_bypass_main >> 8;
      uint8_t o2_lsb_fix = (uint8_t)(o2_fix_bypass_main & 0xFF);

      rxBuf[4] = o2_msb_fix;
      rxBuf[5] = o2_lsb_fix;

    } else if (convModeNox_main == CONV_PROP) {

      uint16_t o2 = ((uint16_t)rxBuf[4] << 8) | rxBuf[5];

      o2 =  o2 * o2_prop_main;

      uint8_t o2_msb_prop = o2 >> 8;
      uint8_t o2_lsb_prop = (uint8_t)(o2 & 0xFF);

      rxBuf[3] = o2_msb_prop;
      rxBuf[4] = o2_lsb_prop;

    }//else if(convMode == CONV_MAP){
    //
    //}
  }else if (printTempReached){
    if (rxId == 0x264){
      DEBUG.print(F("PreCata-Nox: "));
      printBool(rxBuf[1] == 0x80);
      DEBUG.print(F(" PostCata-Nox: "));
      printBool(rxBuf[0] == 0x80);
      DEBUG.println();
    }
  }

  // if CAN bridge is active. Send all CAN Rx frames to other CAN channel
  if (canBridge) {
    if (chNum == 1) {
      sendFrame(&CAN2, rxId, extId, len, rxBuf);
    } else if (chNum == 2) {
      sendFrame(&CAN1, rxId, extId, len, rxBuf);
    }
  }

  if (((rxId & 0x1FFF00FF) == 0x18DA0010) || ((rxId & 0x1FFFFFFF) == 0x18DB3310) || (rxId == obdCanIdStd_ECU)) {
    pidRead(&sidpr, &pId, &payloadSize, rxObdBuf, rxBuf, &currPos, &finish);

    if (finish) {
      pidMng(&sidpr, &pId, &payloadSize, rxObdBuf);
      waitingObdReply = false;
    }
  }

  if (!finish) {
    if (askNoxObd) {
      sendFrame(&CAN1, obdCanIdStd, false, 8, flowStatusFrame);
    }
  }


}

bool sendFrame(MCP_CAN* canCh, uint32_t id, uint8_t ext, uint8_t len, uint8_t *payload) {

  bool result = false;
  byte sndStat = canCh->sendMsgBuf(id, ext, len, payload);
  if (sndStat != CAN_OK) {
    Serial.println(F("ERROR SendFrame: "));
  } else {
    result = true;
  }

  // Check CAN channel:
  uint8_t chNum = 0;
  if (canCh == &CAN1) {
    chNum = 1;
  } else if (canCh == &CAN2) {
    chNum = 2;
  }

  if (canPrint || (can1Print && (chNum == 1)) || (can2Print && (chNum == 2))) {
    // Print timestamp:
    sprintf(msgString, "%f\t", (float)millis() / 1000);
    DEBUG.print(msgString);

    if (ext) {
      // Extended CAN ID:
      sprintf(msgString, "%d\tx%.8lX\tTx\t", chNum, (id & 0x1FFFFFFF));
    } else {
      // Standard CAN ID:
      sprintf(msgString, "%d\t%.3lX\tTx\t", chNum, id);
    }
    DEBUG.print(msgString);

    // Check RTR bit and determine if message is a remote request frame.
    // datatype:
    // r == remote_frame
    // d == datatype
    if ((id & 0x40000000) == 0x40000000) {
      sprintf(msgString, "r\t%d\t", len);
      DEBUG.print(msgString);
    } else {
      sprintf(msgString, "d\t%d\t", len);
      DEBUG.print(msgString);

      for (byte i = 0; i < len; i++) {
        if (payload[i] < 0x10) DEBUG.print('0');
        DEBUG.print(payload[i], HEX);
        DEBUG.print(" ");
      }
    }
    DEBUG.println();
  }
  return result;

  //return false;
  //}
}

void printAsc(uint8_t chNum, uint8_t extId) {
  // Print timestamp:
  sprintf(msgString, "%f\t", (float)millis() / 1000);
  DEBUG.print(msgString);

  if (extId) {
    // Extended CAN ID:
    sprintf(msgString, "%d\tx%.8lX\tRx\t", chNum, (rxId & 0x1FFFFFFF));
  } else {
    // Standard CAN ID:
    sprintf(msgString, "%d\t%.3lX\tRx\t", chNum, rxId);
  }
  DEBUG.print(msgString);

  // Check RTR bit and determine if message is a remote request frame.
  // datatype:
  // r == remote_frame
  // d == datatype
  if ((rxId & 0x40000000) == 0x40000000) {
    sprintf(msgString, "r\t%d\t", len);
    DEBUG.print(msgString);
  } else {

    sprintf(msgString, "d\t%d\t", len);
    DEBUG.print(msgString);

    for (byte i = 0; i < len; i++) {
      if (rxBuf[i] < 0x10) DEBUG.print('0');
      DEBUG.print(rxBuf[i], HEX);
      DEBUG.print(" ");
    }
  }
  DEBUG.println();
}

void broadcastUiData(void) {
  
  convModeNox_main = getConversionModeNox();
  DEBUG.print("convModeNox: ");
  DEBUG.println(convModeNox_main);

  switch (convModeNox_main) {
    case CONV_DIS:
      broadcastJsonById("conv_mode_nox", "CONV_DIS");
      break;
    case CONV_FIX:
      broadcastJsonById("conv_mode_nox", "CONV_FIX");
      break;
    case CONV_PROP:
      broadcastJsonById("conv_mode_nox", "CONV_PROP");
      break;
    case CONV_MAP:
      broadcastJsonById("conv_mode_nox", "CONV_MAP");
      break;
    default:
      broadcastJsonById("conv_mode_nox", "UNKNOWN");
      break;
  }
  
  convModeO2_main = getConversionModeO2();
  DEBUG.print("convModeO2: ");
  DEBUG.println(convModeO2_main);

  switch (convModeO2_main) {
    case CONV_DIS:
      broadcastJsonById("conv_mode_o2", "CONV_DIS");
      break;
    case CONV_FIX:
      broadcastJsonById("conv_mode_o2", "CONV_FIX");
      break;
    case CONV_PROP:
      broadcastJsonById("conv_mode_o2", "CONV_PROP");
      break;
    case CONV_MAP:
      broadcastJsonById("conv_mode_o2", "CONV_MAP");
      break;
    default:
      broadcastJsonById("conv_mode_o2", "UNKNOWN");
      break;
  }
  
  nox_fix_bypass_main = getNoxFixBypass();
  o2_fix_bypass_main = getO2FixBypass();
  broadcastJsonById("o2_fix_bypass", o2_fix_bypass_main);
  broadcastJsonById("nox_fix_bypass", nox_fix_bypass_main);

  nox_prop_main = getNoxProp();
  o2_prop_main = getO2Prop();
  broadcastJsonById("nox_prop", (double)nox_prop_main);
  broadcastJsonById("o2_prop", (double)o2_prop_main);
}























// DEBUG - Serial interface
void debugCmdMng(void) {
  if (DEBUG.available()) {
    char c = DEBUG.read();
    if (c == '?') {
      DEBUG.print(F("\n== UART MENU ==\n"));
      DEBUG.print(F("\ni: ESP32 Info "));
      DEBUG.print(F("\nb: CAN bridge: "));
      printBool(canBridge);
      
      DEBUG.print(F("\n\np: Print CAN verbose: "));
      printBool(canPrint);      
      DEBUG.print(F("\n1: Print CAN CH1 verbose: "));
      printBool(can1Print);
      DEBUG.print(F("\n2: Print CAN CH2 verbose: "));
      printBool(can2Print);
      
      DEBUG.println(F("\n\no: Request supported OBD PId"));
      DEBUG.println(F("n: Request OBD NOx (PId 0x83)"));

      DEBUG.println(F("c: Print conditions reached: "));
      printBool(printTempReached);
      DEBUG.println();
      
      DEBUG.print(F("\nt: Send periodic test frame: "));
      printBool(testFrameTime != 0);
      DEBUG.println();
    } else if (c == 'i'){
      printEspInfo();
    } else if (c == 'b') {
      DEBUG.print("CAN Bridge: ");
      canBridge = !canBridge;
      printBool(canBridge);
      DEBUG.println();
    } else  if (c == 'p') {
      DEBUG.print("CAN print: ");
      canPrint = !canPrint;
      printBool(canPrint);
      DEBUG.println();
    } else  if (c == '1') {
      DEBUG.print("CAN1 print: ");
      can1Print = !can1Print;
      printBool(can1Print);
      DEBUG.println();
    } else  if (c == '2') {
      DEBUG.print("CAN2 print: ");
      can2Print = !can2Print;
      printBool(can2Print);
      DEBUG.println();
    } else  if (c == 'n') {
      DEBUG.print("OBD NOx req: ");
      askNoxObd = !askNoxObd;
      printBool(askNoxObd);
      DEBUG.println();
    } else  if (c == 'o') {
      DEBUG.println(F("Req OBD supported PId"));
      askSupportedPid = true;
    } else if (c == 'c') {
      DEBUG.print(F("Print conditions reached: "));
      printTempReached = !printTempReached;
      printBool(can2Print);
      DEBUG.println();
    } else  if (c == 'o') {
      if (testFrameTime == 0){
        testFrameTime = 1000;
      }else{
        testFrameTime = 0;
      }
    }
  }
}

void printEspInfo(void){
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
}

void printBool(bool b){
  if(b){
    DEBUG.print(F("True"));
  }else{
    DEBUG.print(F("False"));
  }
}

void sendTestFrame(void) {
  // send data:  ID = 0x555, Standard CAN Frame, Data length = 8 bytes, 'testData' = array of data bytes to send
  sendFrame(&CAN1, 0x555, 0, 8, testData);
  sendFrame(&CAN2, 0x555, 0, 8, testData);
}
