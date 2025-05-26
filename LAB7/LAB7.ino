#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Variables para lo calculos
float radio = 0.1; //Radio de la rueda en metros
float VAMradsec = 10.5; // Velocidad angular del motor en radianes por segundo
float RT = 5.2; // Relacion de la transmision (motor a rueda)
float VARradsec = 0.0;

//Aletorios
int number_max = 50;
int number_data = 0;

// --- Datos de tu red WiFi ---
const char* ssid = "DASC";
const char* password = "44034403";

// --- Datos del Broker Mosquitto en la Raspberry Pi ---

const char* mqtt_server = "192.168.243.235";
const int mqtt_port = 1883; // Puerto estándar de MQTT
const char* mqtt_user = ""; // Si configuraste usuario en Mosquitto
const char* mqtt_password = ""; // Si configuraste contraseña en Mosquitto

// --- Tópico MQTT para publicar ---

const char* publish_topic = "motor/data";
const char* subscribe_topic = "motor/command";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long msgInterval = 500; // Intervalo de envío en milisegundos (5 segundos)

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());

}
  void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      // Puedes suscribirte a tópicos aquí si lo deseas
      client.subscribe("motor/command");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");

  // Convert to string
  String jsonStr;
  for (unsigned int i = 0; i < length; i++) {
    jsonStr += (char)payload[i];
  }
  Serial.println(jsonStr);

  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    Serial.println("Error al parsear JSON");
    return;
  }

  float rpm = doc["rpm"];
  float vl = doc["vl"];
  int gear = doc["gear"];

  Serial.printf("rpm: %.2f | vl: %.2f | gear: %d\n", rpm, vl, gear);
}

// Calcular RPM
float calcularRPM(){
  // velocidad angular de la rueda (rad/seg) = velocidad angular del motor (rad/seg)/relacion de transmision
  VARradsec = VAMradsec / RT;

  // rps = velocidad angular de la rueda (rad/seg) / (2 * PI)
  float rps = VARradsec / (2 * PI);

  // rps = rps * 60
  float rpm = rps* 60;

  return rpm;
}

//Calculo de velocida lineal
float calcularV(){
  float VARradsec = VAMradsec / RT;
  float vl = VARradsec * radio;
  return vl;
}


void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  long now = millis();

  if (now - lastMsg > msgInterval) {
    lastMsg = now;

    float rpm = 0.0;
    float vl = 0.0;

    if(number_data < number_max){
      // Calcular RPM y velocidad lineal
      rpm = calcularRPM();
      vl = calcularV();
      number_data++;
    }
    else{
      rpm = random(50, 150) / 10.0; //Rango: 5.0 a 15.0 rad/s (1 decimal)
      vl = random(40, 70) / 10.0; // Rango: 4.0 a 7.0 (1 decimal)
    };

    // Crear el documento JSON
    StaticJsonDocument<200> jsonDocument;
    jsonDocument["rpm"] = rpm;
    jsonDocument["vl"] = vl;

    // Convertir el documento JSON a una cadena
    String jsonString;
    serializeJson(jsonDocument, jsonString);

    Serial.print("Publicando datos del motor: ");
    Serial.println(jsonString);
    client.publish("motor/data", jsonString.c_str());
  }
}