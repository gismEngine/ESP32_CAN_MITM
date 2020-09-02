//#include <ArduinoJson.h>
//
//static const uint32_t SIZE_X = 10;
//uint32_t axisX[SIZE_X] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//
//static const uint32_t SIZE_Y = 10;
//uint32_t axisY[SIZE_Y] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//
//float convMap[SIZE_X][SIZE_Y] = { {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a}, 
//                                  {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a},
//                                  {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a},
//                                  {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a},
//                                  {0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a},
//                                  {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a},
//                                  {0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a},
//                                  {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a},
//                                  {0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a},
//                                  {0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa}};
//
//uint8_t sizeX = 0;
//uint8_t sizeY = 0;
//
//
//void setup() {
//  // put your setup code here, to run once:
//  Serial.begin(115200);
//  Serial.println("MAP TEST");
//
//  initMaps();
//  Serial.println("INIT DONE:");
//}
//
//void loop() {
//  
//  testInterpolateSimp();
//  
//  Serial.println("::::::::::::::::::::::");
//  
//  Serial.println("interpolateFull(2421, 299) - OK: 268.75");
//  Serial.println(interpolateFull(2421, 299));
//
//  Serial.println("interpolateFull(5196, 299) - OK: 561.89");
//  Serial.println(interpolateFull(5196, 299));
//
//  Serial.println("interpolateFull(5196, 554) - OK: 12");
//  Serial.println(interpolateFull(5196, 554));
//
//  Serial.println("interpolateFull(2250, 525) - OK: 19.25");
//  Serial.println(interpolateFull(2250, 525));
//
//  Serial.println("interpolateFull(2250, 406) - OK: 25.83");
//  Serial.println(interpolateFull(2250, 406));
//
//  Serial.println("interpolateFull(0, 299) - OK: 0");
//  Serial.println(interpolateFull(0, 299));
//
//  Serial.println("interpolateFull(2250, 0) - OK: 212.5");
//  Serial.println(interpolateFull(2250, 0));
//  
//  while(1){
//    delay(20);
//  }
//
//}
//
//
//
//float interpolateFull(uint32_t x, uint32_t y){
//
//  Serial.print("x: ");
//  Serial.print(x);
//  Serial.print(" y: ");
//  Serial.println(y);
//
//
//  float v = 0.0;
//
//  uint8_t i_x0 = 0;
//  uint8_t i_x1 = 0;
//
//  uint8_t i = 0;
//  for (i = 0; i < sizeX; i++){
//    if (x < axisX[i]){
//      break;
//    }
//  }
//  if(i == sizeX){
//
//    Serial.println("(i == sizeX)");
//    
//    i_x0 = sizeX - 1;
//    i_x1 = sizeX - 1;
//  }else if(i == 0){
//    
//    Serial.println("(i == 0)");
//    
//    i_x0 = 0;
//    i_x1 = 0;
//  }else{
//    
//    Serial.println("else");
//    
//    i_x0 = i - 1;
//    i_x1 = i;
//  }
//
//
//  Serial.print("i_x0: ");
//  Serial.println(i_x0);
//  Serial.print("i_x1: ");
//  Serial.println(i_x1);
//  
//
//  for (i = 0; i < sizeY; i++){
//    if (y < axisY[i]){
//      break;
//    }
//  }
//  
//  Serial.print("i_Y: ");
//  Serial.println(i);
//
//  
//  if(i == sizeY){
//
//    Serial.println("(i == sizeY)");
//    
//    float z0 = convMap[sizeY - 1][i_x0];
//    float z1 = convMap[sizeY - 1][i_x1];
//    
//    v = interpolateSimp(axisX[i_x0], z0, axisX[i_x1], z1, x);
//    return v;
//    
//  }else if(i == 1){
//
//    Serial.println("(i == 0)");
//    
//    float z0 = convMap[0][i_x0];
//    float z1 = convMap[0][i_x1];
//    
//    v = interpolateSimp(axisX[i_x0], z0, axisX[i_x1], z1, x);
//    return v;
//    
//  }else{
//
//    Serial.println("else");
//
//    Serial.print("axisX[i_x0]: ");
//    Serial.println(axisX[i_x0]);
//    Serial.print("axisX[i_x1]: ");
//    Serial.println(axisX[i_x1]);  
//    
//    float z00 = convMap[i - 1][i_x0];
//    float z01 = convMap[i - 1][i_x1];
//
//    Serial.print("z00: ");
//    Serial.println(z00);
//    Serial.print("z01: ");
//    Serial.println(z01);
//    
//    float v0 =  interpolateSimp(axisX[i_x0], z00, axisX[i_x1], z01, x);
//
//    Serial.print("v0: ");
//    Serial.println(v0);
//
//    float z10 = convMap[i][i_x0];
//    float z11 = convMap[i][i_x1];
//
//    Serial.print("z10: ");
//    Serial.println(z10);
//    Serial.print("z11: ");
//    Serial.println(z11);
//
//    float v1 =  interpolateSimp(axisX[i_x0], z10, axisX[i_x1], z11, x);
//
//    Serial.print("v1: ");
//    Serial.println(v1);
//
//    v =  interpolateSimp(axisY[i - 1], v0, axisY[i], v1, y);
//
//    Serial.print("v: ");
//    Serial.println(v);
//    
//    return v;
//  }
//
//  return 0.0;
//}
//
//
//void testInterpolateSimp(){
//  
//  float x0 = 1;
//  float y0 = 2;
//  float x1 = 3;
//  float y2 = 6;
//  float x = 2;
//  
//  verboseInterpolateSimp(x0, y0, x1, y2, x);
//  Serial.println("Expected: 4");
//
//  x0 = 5;
//  y0 = 33;
//  x1 = 66;
//  y2 = 123;
//  x = 55;
//  
//  verboseInterpolateSimp(x0, y0, x1, y2, x);
//  Serial.println("Expected: 106.77");
//}
//
//void verboseInterpolateSimp(float x0, float y0, float x1, float y1, float x){
//
//  Serial.print("x0: ");
//  Serial.print(x0);
//
//  Serial.print(" y0: ");
//  Serial.println(y0);
//
//  Serial.print("x1: ");
//  Serial.print(x1);
//
//  Serial.print(" y1: ");
//  Serial.println(y1);
//
//  Serial.print("x: ");
//  Serial.println(x);
//
//  Serial.print("y: ");
//  Serial.println(interpolateSimp(x0,y0,x1, y1, x));
//}
//
//float interpolateSimp(float x0, float y0, float x1, float y1, float x){
//  if (x1 == x0){
//    Serial.println("==");
//    return ((y0 + y1) / 2);
//  }
//  Serial.println("!=");
//  return ((x - x0) * (y1 - y0)) / (x1 - x0) + y0;
//}
//
//
//void initMaps(void){
//    // compute the required size
//  const size_t CAPACITY = JSON_ARRAY_SIZE(250);
//
//  // allocate the memory for the document
//  StaticJsonDocument<CAPACITY> doc;
//
//  // parse a JSON array
//  char JSONMessage[] = "{\"x\":[0,1000,2000,3000, 4000, 5000],\"y\":[0,100,200, 300, 400, 500],\"map\":[[0, 100, 200, 250, 300, 400],[0, 455, 34, 233, 87, 4345],[0, 22.4, 33.5, 34, 5567, 56],[0, 56, 56, 567, 56, 567],[0, 567, 34, 3, 456, 45],[0, 444, 22, 11, 123, 12]]}";
//  deserializeJson(doc, JSONMessage);
//
//  // extract the values
////  JsonArray array = doc.as<JsonArray>();
////  for(JsonVariant v : array) {
////      Serial.println(v.as<int>());
////  }
//
//  sizeX =  doc["x"].size();
//  sizeY =  doc["y"].size();
//
//  Serial.print("X size: ");
//  Serial.println(sizeX);
//
//  Serial.print("Y size: ");
//  Serial.println(sizeY);
//
//  Serial.print("X: ");
//  for (uint8_t i = 0; i < sizeX; i++){
//    uint32_t sensorValue = doc["x"][i];
//    Serial.print(sensorValue);
//    Serial.print(", ");
//
//    axisX[i] = sensorValue;
//  }
//  Serial.println();
//
//  Serial.print("Y: ");
//  for (uint8_t i = 0; i < sizeY; i++){
//    uint32_t sensorValue = doc["y"][i];
//    Serial.print(sensorValue);
//    Serial.print(", ");
//
//    axisY[i] = sensorValue;
//  }
//  Serial.println();
//
//
//  uint8_t arraySizeMap =  doc["map"].size();
//  Serial.print("MAP size: ");
//  Serial.println(arraySizeMap);
//
//  Serial.print("MAP: ");
//
//  uint8_t i = 0;
//  for (i = 0; i < arraySizeMap; i++){
//
//    uint8_t j = 0;
//    // extract the values
//    JsonArray array = doc["map"][i].as<JsonArray>();
//    for(JsonVariant v : array) {
//        float a = v.as<float>();
//        Serial.print(a);
//        Serial.print(", ");
//
//        convMap[i][j] = v;
//        j = j + 1;
//    }
//    Serial.println();
//    
//    if(j !=  sizeX){
//      Serial.println("ERROR MAP Size (X)");
//    }
//    
//  }
//  
//  if(i !=  sizeY){
//      Serial.println("ERROR MAP Size (Y)");
//  }
//  Serial.println();
//}
//
//
//void printBuffersSerial(void){
//    Serial.println("Axis X: ");
//  for (uint8_t i = 0; i < sizeX; i++){
//    Serial.print(axisX[i]);
//    Serial.print(", ");
//  }
//  Serial.println();
//
//
//  Serial.println("FULL BUFFER Axis X: ");
//  for (uint8_t i = 0; i < SIZE_X; i++){
//    Serial.print(axisX[i]);
//    Serial.print(", ");
//  }
//  Serial.println();
//
//  Serial.println("Axis Y: ");
//  for (uint8_t i = 0; i < sizeY; i++){
//    Serial.print(axisY[i]);
//    Serial.print(", ");
//  }
//  Serial.println();
//
//  Serial.println("FULL BUFFER Axis Y: ");
//  for (uint8_t i = 0; i < SIZE_Y; i++){
//    Serial.print(axisX[i]);
//    Serial.print(", ");
//  }
//  Serial.println();
//
//
//
//  Serial.println("MAP: ");
//  for (uint8_t i = 0; i < sizeX; i++){
//    for (uint8_t j = 0; j < sizeY; j++){
//      Serial.print(convMap[i][j]);
//      Serial.print(", ");
//    }
//    Serial.println();
//  }
//  Serial.println();
//
//
//  Serial.println("FULL BUFFER MAP: ");
//  for (uint8_t i = 0; i < SIZE_X; i++){
//    for (uint8_t j = 0; j < SIZE_Y; j++){
//      Serial.print((uint32_t)convMap[i][j], HEX);
//      Serial.print(", ");
//    }
//    Serial.println();
//  }
//  Serial.println();
//}
