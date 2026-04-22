#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char *NETWORK_NAME = "Free Reading WiFi";
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 8, 8);
DNSServer dnsServer;
ESP8266WebServer server(80);

void handleIndex() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Error: index.html not found. Check LittleFS upload.");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void redirectToPortal() {
  String url = "http://" + apIP.toString() + "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nInitializing...");

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(NETWORK_NAME);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleIndex);
  server.on("/index.html", handleIndex);

  server.on("/generate_204", redirectToPortal);
  server.on("/hotspot-detect.html", redirectToPortal);
  server.onNotFound(redirectToPortal);
  server.begin();
  Serial.println("Captive Portal Ready.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
