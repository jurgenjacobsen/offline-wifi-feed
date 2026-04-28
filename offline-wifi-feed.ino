#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char *NETWORK_NAME = "Free Reading WiFi";
const byte DNS_PORT = 53;
IPAddress apIP(8, 8, 8, 8);
DNSServer dnsServer;
ESP8266WebServer server(80);

IPAddress recentIPs[20];
int recentIPCount = 0;

void incrementCounter() {
  IPAddress currentIP = server.client().remoteIP();

  // Check if IP is in the "recently seen" cache
  for (int i = 0; i < recentIPCount; i++) {
    if (recentIPs[i] == currentIP) {
      return; // Already counted this session, skip
    }
  }

  // Add to cache (FIFO)
  if (recentIPCount < 20) {
    recentIPs[recentIPCount++] = currentIP;
  } else {
    for (int i = 0; i < 19; i++) recentIPs[i] = recentIPs[i + 1];
    recentIPs[19] = currentIP;
  }

  int count = 0;
  if (LittleFS.exists("/counter.txt")) {
    File file = LittleFS.open("/counter.txt", "r");
    if (file) {
      count = file.readString().toInt();
      file.close();
    }
  }

  count++;

  File file = LittleFS.open("/counter.txt", "w");
  if (file) {
    file.print(count);
    file.close();
    Serial.print("New Unique Visit! Total Count: ");
    Serial.println(count);
  }
}

void handleIndex() {
  incrementCounter();
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
  } else {
    if (LittleFS.exists("/counter.txt")) {
      File file = LittleFS.open("/counter.txt", "r");
      if (file) {
        Serial.print("Current Saved Count: ");
        Serial.println(file.readString());
        file.close();
      }
    }
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
