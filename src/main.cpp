#include <WiFi.h>  
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

extern "C" {
#include "EngTrModel.h"
}

#define RXD2 16   // UART2 RX
#define TXD2 17   // UART2 TX

// --- Datos de tu red WiFi ---
// const char* ssid = "Bacalao con jamon";
// const char* password = "123456789";
const char* ssid = "DASC";
const char* password = "44034403";

// --- Datos del Broker Mosquitto en la Raspberry Pi ---

// const char* mqtt_server = "192.168.72.235";
const char* mqtt_server = "192.168.202.251";
const int mqtt_port = 1883; // Puerto estándar de MQTT
const char* mqtt_user = ""; // Si configuraste usuario en Mosquitto
const char* mqtt_password = ""; // Si configuraste contraseña en Mosquitto

// --- Tópico MQTT para publicar ---

const char* publish_topic = "motor/data";
const char* subscribe_topic = "motor/command";


// --- Model Engine ---
float Throttle = 5;
float BrakeTorque = 0;
bool callback_status = false;
float acc = 0;
float brake = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long msgInterval = 500; // Intervalo de envío en milisegundos (5 segundos)
long now = millis();

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

  acc = doc["acceleration"];
  brake = doc["brake"];
  int callback_status_char = doc["callback_status"];
  if(callback_status_char == 1)
    callback_status = true;
  else callback_status = false;

  Serial.printf("Acelerador: %.2f | Frenos: %.2f\n", acc, brake);
}

void process_uart2_input(float &Throttle,float &BrakeTorque) {
  float Throttle_ = -1.0;
  int BrakeTorque_ = -1.0;

  // Read Serial
  if(Serial2.available())
  {
    String line = Serial2.readStringUntil('E');
    line.trim();
    Serial.print("Received: ");
    Serial.println();
    if(line.startsWith("I")) {
      // Scan received string
      if (sscanf(line.c_str(), "I%f,%d,", &Throttle_, &BrakeTorque_) == 2) {
        if(BrakeTorque_ > 0) BrakeTorque = 2000;
        else BrakeTorque = 0;
        Throttle = ((Throttle_ / 40.0f) * 400.0f) + 5;
        // Print received data
        Serial.print("Received line: ");
        Serial.println(line);
      }
    }
  }
}

void send_MQTT(float rpm_, float vl_, float gear_){
  now = millis();

  // MQTT
  if(now - lastMsg > msgInterval){
    StaticJsonDocument<200> jsonDocument;

    jsonDocument["rpm"] = rpm_;
    jsonDocument["vl"] = vl_;
    jsonDocument["gear"] = gear_;

    // Convertir el documento JSON a una cadena
    String jsonString;
    serializeJson(jsonDocument, jsonString);

    lastMsg = now;
    Serial.println(jsonString);
    client.publish("motor/data", jsonString.c_str());
  }
}

void process_uart2_output(float rpm, float vl, float gear){
  // Convert float to int
  int rpm_i = (int)(rpm*100);
  int vl_i = (int)(vl*100);
  int gear_i = (int)(gear);

  //Convet int(data) to string or char
  String rpm_c1 = String(rpm_i / 100);
  String rpm_c2 = String(rpm_i % 100);
  String vl_c1 = String(vl_i / 100);
  String vl_c2 = String(vl_i % 100);
  char gear_c = '0' + gear_i;

  // Send message to STM
  Serial2.write("I"); //Initial message
  for (int i = 0; i < rpm_c1.length(); i++) {
    Serial2.write(rpm_c1[i]);  // envía cada carácter individualmente
  }
  Serial2.write(","); // Its like a point (.)
  for (int i = 0; i < rpm_c2.length(); i++) {
    Serial2.write(rpm_c2[i]);  // envía cada carácter individualmente
  }
  Serial2.write(",");
  for (int i = 0; i < vl_c1.length(); i++) {
    Serial2.write(vl_c1[i]);  // envía cada carácter individualmente
  }
  Serial2.write(","); // Its like a point (.)
  for (int i = 0; i < vl_c2.length(); i++) {
    Serial2.write(vl_c2[i]);  // envía cada carácter individualmente
  }
  Serial2.write(",");
  Serial2.write(gear_c);
  Serial2.write(",");
  Serial2.write("E"); // End message

  // Send confirm to serial USB
  Serial.print("Datos enviados: ");
  Serial.print("I");
  Serial.print(rpm);
  Serial.print(",");
  Serial.print(vl);
  Serial.print(",");
  Serial.print(gear);
  Serial.print(",");
  Serial.println("E");
}

void setup() {
  // Serial initialize
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(10);
  // Engine Model initialize
  EngTrModel_initialize();

  // Setup wifi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("ESP32 listo para recibir datos por UART");
}

void loop() {
  Throttle = 5;
  BrakeTorque = 0;

  //MQTT
  if (!client.connected()) {
   reconnect();
  }
  client.loop();

  process_uart2_input(Throttle, BrakeTorque);
  delay(150);

  if(callback_status){
    Throttle = acc;
    BrakeTorque = brake;
  }

   // Motor model process
  EngTrModel_U.Throttle = Throttle;
  EngTrModel_U.BrakeTorque = BrakeTorque;
  EngTrModel_step();

  float rpm = EngTrModel_Y.EngineSpeed;
  float vl = EngTrModel_Y.VehicleSpeed;
  float gear = EngTrModel_Y.Gear;

  process_uart2_output(rpm, vl, gear);

  send_MQTT(rpm, vl, gear);
}
