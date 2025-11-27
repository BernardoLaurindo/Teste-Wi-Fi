#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Inteli.Iot";
const char* WIFI_PASS = "%(Yk(sxGMtvFEs.3";
const char* UBIDOTS_TOKEN = "BBUS-i3mtE5fd5ahnsTjAuDJ7ILvj0pJOab";
const char* DEVICE_LABEL = "teste-wifi";
const char* MQTT_SERVER = "industrial.api.ubidots.com";
const uint16_t MQTT_PORT = 1883;

const unsigned long PUBLISH_INTERVAL_MS = 1000UL;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastPublish = 0;

String ubidotsTopic() {
  String t = "/v1.6/devices/";
  t += DEVICE_LABEL;
  return t;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("Conectando à rede WiFi '%s'...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000UL) {
      Serial.println("\nTempo esgotado ao conectar WiFi, tentando novamente...");
      start = millis();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }
  Serial.println();
  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  if (mqttClient.connected()) return;

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.print("Conectando ao MQTT (Ubidots)...");
  String clientId = DEVICE_LABEL;
  clientId += "-";
  clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

  if (mqttClient.connect(clientId.c_str(), UBIDOTS_TOKEN, NULL)) {
    Serial.println(" conectado ao MQTT.");
  } else {
    Serial.print(" falha. rc=");
    Serial.print(mqttClient.state());
    Serial.println(" - Retentando em breve...");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.println("Setup completo.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  } else {
    mqttClient.loop();
  }

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;

    long rssi = WiFi.RSSI();

    Serial.printf("[%lu] RSSI: %ld dBm\n", now, rssi);

    String payload = "{\"sinal\": " + String(rssi) + "}";
    String topic = ubidotsTopic();

    boolean ok = false;
    if (mqttClient.connected()) {
      ok = mqttClient.publish(topic.c_str(), payload.c_str());
    }

    if (ok) {
      Serial.println("Publicado em Ubidots -> " + topic + ": " + payload);
    } else {
      Serial.println("Falha ao publicar. MQTT conectado? " + String(mqttClient.connected()));
    }
  }
  
  delay(10);
}
