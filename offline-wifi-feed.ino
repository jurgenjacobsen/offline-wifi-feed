#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

const byte DNS_PORT = 53;
// We use 8.8.8.8 to trick Android devices that hardcode Google DNS
IPAddress apIP(8, 8, 8, 8); 
DNSServer dnsServer;
ESP8266WebServer server(80);

const char index_html[] PROGMEM = R"raw(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Portal</title>
<style>body{font-family:sans-serif;text-align:center;padding:50px;}h1{color:#007bff;}</style>
</head><body>
<h1>Portal Connected</h1>
<p>You are now connected to the D1 Mini local network.</p>
</body></html>)raw";

// This is the "Magic" function. It tells the phone: 
// "Whatever you were looking for, it's actually at my IP."
void redirectToPortal() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", ""); 
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  // We set the gateway to 8.8.8.8 to catch hardcoded Google DNS requests
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Lolin-D1-Mini"); 

  // DNS "*" means all domains go to the D1 Mini
  dnsServer.start(DNS_PORT, "*", apIP);

  // 1. Handle the main page
  server.on("/", handleRoot);

  // 2. Catch common "Connectivity Check" URLs and REDIRECT them
  server.on("/generate_204", redirectToPortal);       // Android
  server.on("/hotspot-detect.html", redirectToPortal); // iOS
  server.on("/canonical.html", redirectToPortal);      // iOS
  server.on("/success.txt", redirectToPortal);        // OSX
  server.on("/ncsi.txt", redirectToPortal);           // Windows

  // 3. Catch-all: If the phone asks for anything else (like google.com)
  server.onNotFound(redirectToPortal);

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}