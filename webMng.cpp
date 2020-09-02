#include <DNSServer.h>
#include "Arduino.h"
#include "webMng.h"

#include "credentials.h"

DNSServer dnsServer;                                 // DNS for the captive portal
WebServer server(80);                                // Web server. Only one client! To be updated to ESPAsyncWebServer?

IPAddress local_IP_STA(192, 168, 1, 39);
IPAddress local_IP_AP(192, 168, 1, 1);

IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

WebMng webMng;

WebMng::WebMng(){
}

void WebMng::begin(const char *fwName, const char *fwVer){
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, LOW);
  
  _fwName = fwName;
  _fwVer = fwVer;

  if (SPIFFS.begin()) {
    Serial.println(F("Mounted file system"));
  } else {
    Serial.println(F("Failed to mount FS"));
  }

  //clean FS, for testing
  //SPIFFS.format();

  initWifi();

  server.on(F("/"), [](){
     server.sendHeader("Location","/index.html");        // Add a header to respond with a new location for the browser to go to the home page again
     server.send(303);                                   // Send it back to the browser with an HTTP status 303 (See Other) to redirect
  } );
  
  server.on(F("/ip"), [](){
    server.send(200, "text/plain", WiFi.softAPIP().toString().c_str());
  });

  server.on(F("/name"), [&](){
    server.send(200, "text/plain", _fwName);
  } );

  server.on(F("/version"), [&](){
    server.send(200, "text/plain", _fwVer);
  } );

  server.onNotFound([&]() {                    // If the client requests any URI
    if (!handleFileRead(server.uri()))         // send it if it exists
    
//    // Redirect all not found to localhost/404.html.
//    // This is captive portal
//    // 303: redirect
//    server.sendHeader("Location","/404.html");
//    server.send(303); 

    // Redirect to captive portal (tool control pannel --> index.html)
    server.sendHeader("Location","/index.html");
    server.send(303);           
  });
  
  server.begin();
  Serial.println(F("Web-Server started"));

  digitalWrite(LED_WIFI, HIGH);
}


void WebMng::initWifi(void){
  
  #if defined (WIFI_AP_MODE)
  
    Serial.print(F("Setting soft-AP ("));
    Serial.print(_fwName);
    Serial.print(F(")..."));

    if (!WiFi.config(local_IP_AP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println(F("AP Failed to configure "));
    }
    
    WiFi.mode(WIFI_AP_STA);
    boolean result = WiFi.softAP(_fwName);
    delay(100);                                           // Delay is critic before softAPConfig!
    
    WiFi.softAPConfig(local_IP_AP, gateway, subnet);     // Config MUST be after WiFi.softAP(**)
    if(result == true){
      Serial.println(F("OK"));
    }else{
      Serial.println(F("FAIL!"));
    }  
    Serial.print(F("Local IP: "));
    Serial.println(WiFi.softAPIP());
        
    // Setup DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
  
  #elif defined (WIFI_STA_MODE)

    Serial.print(F("Connecting to "));
    Serial.println(ssid);
//    Serial.print(F("Pass: "));
//    Serial.println(password);

//    if (!WiFi.config(local_IP_STA, gateway, subnet, primaryDNS, secondaryDNS)) {
//      Serial.println(F("STA Failed to configure"));
//    }

    WiFi.mode(WIFI_STA);
    delay(10);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  
    Serial.println();
    Serial.println(F("WiFi connected!"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("ESP Mac Address: "));
    Serial.println(WiFi.macAddress());
    Serial.print(F("Subnet Mask: "));
    Serial.println(WiFi.subnetMask());
    Serial.print(F("Gateway IP: "));
    Serial.println(WiFi.gatewayIP());
    Serial.print(F("DNS: "));
    Serial.println(WiFi.dnsIP());
  #endif
}

void WebMng::manage(){
  server.handleClient();
  dnsServer.processNextRequest();
  
  if (_t_ws + 1000 < millis()){
    //broadcastJson();
    _t_ws = millis();
  }
}

String WebMng::getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool WebMng::handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.print(F("WEB: FileRead: "));
  Serial.println(path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println(F("File Not Found"));
  return false;                                         // If the file doesn't exist, return false
}

// Format SPIFFS (for development)
void formatFileSystem(){
  SPIFFS.format();
}
