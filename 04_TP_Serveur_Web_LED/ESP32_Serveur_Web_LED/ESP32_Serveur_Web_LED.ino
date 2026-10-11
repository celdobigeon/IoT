// TP IoT - Piloter une LED avec un serveur web sur ESP32
// Carte : DOIT ESP32 DEVKIT V1 - LED + résistance 220 ohms sur la broche D26 (GPIO 26)

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "NOM_DU_RESEAU";      // nom du réseau Wi-Fi (2,4 GHz)
const char* password = "MOT_DE_PASSE";   // mot de passe du réseau Wi-Fi

const int LED_PIN = 26;
WebServer server(80);                    // serveur web sur le port 80
bool etat_led = false;

// Construit la page HTML envoyée au navigateur
String page() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width'></head><body>";
  html += "<h1>LED : ";
  html += etat_led ? "allumée" : "éteinte";
  html += "</h1>";
  // Voyant cliquable : il ouvre /off si la LED est allumée, /on si elle est éteinte
  html += etat_led ? "<a href='/off'>" : "<a href='/on'>";
  html += "<div style='width:80px;height:80px;border-radius:50%;border:3px solid #333;background:";
  html += etat_led ? "#ff2020;box-shadow:0 0 30px #ff2020" : "#555";
  html += "'></div></a>";
  html += "<p>Appuyez sur le voyant pour ";
  html += etat_led ? "éteindre" : "allumer";
  html += " la LED.</p>";
  html += "</body></html>";
  return html;
}

// Appelée quand le navigateur demande l'adresse /
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", page());
}

// Appelée quand le navigateur demande l'adresse /on
void handleOn() {
  digitalWrite(LED_PIN, HIGH);
  etat_led = digitalRead(LED_PIN);
  server.send(200, "text/html; charset=utf-8", page());
}

// Appelée quand le navigateur demande l'adresse /off
void handleOff() {
  digitalWrite(LED_PIN, LOW);
  etat_led = digitalRead(LED_PIN);
  server.send(200, "text/html; charset=utf-8", page());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);            // connexion au Wi-Fi
  Serial.print("Connexion au Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());        // adresse à taper dans le navigateur

  server.on("/", handleRoot);            // adresse -> fonction
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();                        // démarrage du serveur web
}

void loop() {
  server.handleClient();                 // traite les requêtes des navigateurs
}
