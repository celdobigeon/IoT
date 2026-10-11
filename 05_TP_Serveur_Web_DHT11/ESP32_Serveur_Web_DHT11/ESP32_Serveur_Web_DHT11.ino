// TP IoT - Afficher la température et l'humidité sur une page web hébergée par l'ESP32
// Carte : DOIT ESP32 DEVKIT V1 - module DHT11 : S sur D4 (GPIO 4), + sur 3V3, - sur GND
// Bibliothèque à installer : « DHT sensor library » (Adafruit)

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

const char* ssid = "NOM_DU_RESEAU";      // nom du réseau Wi-Fi (2,4 GHz)
const char* password = "MOT_DE_PASSE";   // mot de passe du réseau Wi-Fi

const int DHT_PIN = 4;                   // broche DATA = D4
DHT dht(DHT_PIN, DHT11);                 // capteur de type DHT11
WebServer server(80);                    // serveur web sur le port 80

float temperature = 0;                   // dernière mesure valide (°C)
float humidite = 0;                      // dernière mesure valide (%)
bool erreur_capteur = true;              // vrai tant qu'aucune mesure valide n'a été faite
unsigned long derniereMesure = 0;        // instant de la dernière mesure (ms)
const unsigned long PERIODE = 5000;      // une mesure toutes les 5 s

// Historique des mesures, gardé dans la mémoire de la carte (effacé si la carte redémarre)
const int NB_MAX = 360;                  // 360 mesures x 5 s = 30 minutes
float histT[NB_MAX];                     // températures (°C)
float histH[NB_MAX];                     // humidités (%)
unsigned long histTemps[NB_MAX];         // instant de chaque mesure (s depuis le démarrage)
int nbMesures = 0;                       // nombre de mesures enregistrées

// Ajoute une mesure à l'historique
void enregistrer(float t, float h) {
  if (nbMesures == NB_MAX) {             // historique plein : on oublie la mesure la plus ancienne
    for (int i = 1; i < NB_MAX; i++) {
      histT[i - 1] = histT[i];
      histH[i - 1] = histH[i];
      histTemps[i - 1] = histTemps[i];
    }
    nbMesures--;
  }
  histT[nbMesures] = t;
  histH[nbMesures] = h;
  histTemps[nbMesures] = millis() / 1000;
  nbMesures++;
}

// Demande une mesure au capteur et la garde en mémoire
void mesurer() {
  float t = dht.readTemperature();       // température en °C
  float h = dht.readHumidity();          // humidité relative en %
  if (isnan(t) || isnan(h)) {            // isnan() : la mesure a échoué
    erreur_capteur = true;
    Serial.println("Erreur de lecture du DHT11 !");
  } else {
    erreur_capteur = false;
    temperature = t;
    humidite = h;
    enregistrer(t, h);
    Serial.print("Temperature : ");
    Serial.print(t);
    Serial.print(" C   Humidite : ");
    Serial.print(h);
    Serial.println(" %");
  }
}

// Dessine une jauge en demi-cercle (image SVG) : l'arc coloré va de mini à la valeur
String jauge(String titre, float valeur, float mini, float maxi, int decimales, String unite, String couleur) {
  float k = constrain((valeur - mini) / (maxi - mini), 0.001, 1);   // fraction de 0 à 1
  float angle = PI * (1 - k);            // 180° pour mini, 0° pour maxi
  float x = 100 + 80 * cos(angle);       // bout de l'arc coloré
  float y = 100 - 80 * sin(angle);
  String s = "<svg width='200' height='140' viewBox='0 0 200 140'>";
  s += "<path d='M20,100 A80,80 0 0 1 180,100' fill='none' stroke='#ddd' stroke-width='16'/>";
  s += "<path d='M20,100 A80,80 0 0 1 " + String(x, 1) + "," + String(y, 1) + "' fill='none' stroke='" + couleur + "' stroke-width='16'/>";
  s += "<text x='100' y='92' text-anchor='middle' font-size='26' font-weight='bold'>" + String(valeur, decimales) + " " + unite + "</text>";
  s += "<text x='20' y='120' text-anchor='middle' font-size='12'>" + String(mini, 0) + "</text>";
  s += "<text x='180' y='120' text-anchor='middle' font-size='12'>" + String(maxi, 0) + "</text>";
  s += "<text x='100' y='135' text-anchor='middle' font-size='14'>" + titre + "</text>";
  s += "</svg>";
  return s;
}

// Dessine les courbes de l'historique (image SVG) : température de 0 à 50 °C, humidité de 0 à 100 %
String courbe() {
  String s = "<svg width='100%' viewBox='0 0 680 260' style='max-width:680px'>";
  for (int i = 0; i <= 4; i++) {         // 5 lignes horizontales de graduation
    int y = 220 - 50 * i;
    s += "<line x1='40' y1='" + String(y) + "' x2='640' y2='" + String(y) + "' stroke='#ddd'/>";
    s += "<text x='34' y='" + String(y + 4) + "' text-anchor='end' font-size='11' fill='#c2410c'>" + String(i * 12.5, 1) + "</text>";
    s += "<text x='646' y='" + String(y + 4) + "' font-size='11' fill='#2b5fd6'>" + String(i * 25) + "</text>";
  }
  String ptsT = "", ptsH = "";           // listes de points "x,y x,y ..."
  for (int i = 0; i < nbMesures; i++) {
    float x = 40 + 600.0 * i / (NB_MAX - 1);
    ptsT += String(x, 1) + "," + String(220 - histT[i] * 200 / 50, 1) + " ";
    ptsH += String(x, 1) + "," + String(220 - histH[i] * 200 / 100, 1) + " ";
  }
  s += "<polyline points='" + ptsT + "' fill='none' stroke='#c2410c' stroke-width='2'/>";
  s += "<polyline points='" + ptsH + "' fill='none' stroke='#2b5fd6' stroke-width='2'/>";
  s += "<text x='40' y='245' font-size='12' fill='#c2410c'>— température (°C)</text>";
  s += "<text x='640' y='245' text-anchor='end' font-size='12' fill='#2b5fd6'>humidité (%) —</text>";
  s += "</svg>";
  return s;
}

// Construit la page HTML envoyée au navigateur
String page() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width'>";
  html += "<meta http-equiv='refresh' content='5'>";   // le navigateur redemande la page toutes les 5 s
  html += "</head><body style='font-family:sans-serif'>";
  html += "<h1>Station météo</h1>";
  if (erreur_capteur) {
    html += "<p style='color:#c00'><b>Erreur de lecture du DHT11 !</b></p>";
  } else {
    html += jauge("Température", temperature, 0, 50, 1, "°C", "#c2410c");
    html += jauge("Humidité", humidite, 0, 100, 0, "%", "#2b5fd6");
  }
  html += "<p>Dernière mesure il y a " + String((millis() - derniereMesure) / 1000) + " s</p>";
  html += "<h2>Historique (" + String(nbMesures) + " mesures, 30 min au maximum)</h2>";
  html += courbe();
  html += "<p><a href='/mesures.csv'>Télécharger les mesures (fichier CSV)</a></p>";
  html += "</body></html>";
  return html;
}

// Écrit un nombre avec une virgule (format attendu par un tableur en français)
String virgule(float v) {
  String s = String(v, 1);
  s.replace(".", ",");
  return s;
}

// Appelée quand le navigateur demande l'adresse /
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", page());
}

// Appelée quand le navigateur demande l'adresse /mesures.csv : envoie l'historique en fichier CSV
void handleCSV() {
  String csv = "\xEF\xBB\xBF";           // marque UTF-8 : le tableur affiche bien les accents
  csv += "temps (s);temperature (°C);humidite (%)\n";
  for (int i = 0; i < nbMesures; i++) {
    csv += String(histTemps[i]) + ";" + virgule(histT[i]) + ";" + virgule(histH[i]) + "\n";
  }
  server.sendHeader("Content-Disposition", "attachment; filename=mesures.csv");   // à télécharger
  server.send(200, "text/csv; charset=utf-8", csv);
}

void setup() {
  Serial.begin(115200);
  dht.begin();                           // démarrer le capteur

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
  server.on("/mesures.csv", handleCSV);
  server.begin();                        // démarrage du serveur web

  mesurer();                             // première mesure
  derniereMesure = millis();
}

void loop() {
  server.handleClient();                 // traite les requêtes des navigateurs

  // une mesure toutes les 5 s, sans bloquer le serveur web
  if (millis() - derniereMesure >= PERIODE) {
    derniereMesure = millis();
    mesurer();
  }
}
