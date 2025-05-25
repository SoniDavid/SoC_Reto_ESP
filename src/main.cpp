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
const char* ssid = "Bacalao con jamon";
const char* password = "e7cx9xyyh36gdbr";

// --- Datos del Broker Mosquitto en la Raspberry Pi ---

const char* mqtt_server = "192.168.62.251";
const int mqtt_port = 1883; // Puerto estándar de MQTT
const char* mqtt_user = ""; // Si configuraste usuario en Mosquitto
const char* mqtt_password = ""; // Si configuraste contraseña en Mosquitto

// --- Tópico MQTT para publicar ---

const char* publish_topic = "motor/data";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long msgInterval = 100; // Intervalo de envío en milisegundos (5 segundos)

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
      // client.subscribe("esp32/control");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void process_uart2_input() {
    float rpm_ = 0;
    float vl_ = 0;
    float gear_ = 0;

     if(Serial2.available())
    {
      String line = Serial2.readStringUntil('E');
      Serial.print("Received line: ");
      Serial.println(line);
      if(line.startsWith("I")) {
        if (sscanf(line.c_str(), "I%f,%f,%f,", &rpm_, &vl_, &gear_) == 3) {
          StaticJsonDocument<200> jsonDocument;

          jsonDocument["rpm"] = rpm_;
          jsonDocument["vl"] = vl_;
          jsonDocument["gear"] = gear_;

          // Convertir el documento JSON a una cadena
          String jsonString;
          serializeJson(jsonDocument, jsonString);
          Serial.print(rpm_);
          Serial.print(vl_);
          Serial.println(gear_);

          //Serial.println(jsonString);
          //client.publish("motor/data", jsonString.c_str());
        }
      }
      //Serial.print(rpm);
      //Serial.print(vl);
      //Serial.println(gear);
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    delay(10);
    EngTrModel_initialize();

    Serial.println("ESP32 listo para recibir datos por UART");
}

void loop() {
    process_uart2_input();

    EngTrModel_U.Throttle = 50.0;
    EngTrModel_U.BrakeTorque = 0.0;

    EngTrModel_step();

    float rpm = EngTrModel_Y.VehicleSpeed;
    float vl = EngTrModel_Y.EngineSpeed;
    float gear = EngTrModel_Y.Gear;

    int rpm_i = (int)(rpm*100);
    int vl_i = (int)(vl*100);
    int gear_i = (int)(gear);

    String rpm_c1 = String(rpm_i / 100);
    String rpm_c2 = String(rpm_i % 100);
    String vl_c1 = String(vl_i / 100);
    String vl_c2 = String(vl_i % 100);
    char gear_c = '0' + gear_i;

    Serial2.write("I");
    for (int i = 0; i < rpm_c1.length(); i++) {
      Serial2.write(rpm_c1[i]);  // envía cada carácter individualmente
    }
    Serial2.write(",");
    for (int i = 0; i < rpm_c2.length(); i++) {
      Serial2.write(rpm_c2[i]);  // envía cada carácter individualmente
    }
    Serial2.write(",");
    for (int i = 0; i < vl_c1.length(); i++) {
      Serial2.write(vl_c1[i]);  // envía cada carácter individualmente
    }
    Serial2.write(",");
    for (int i = 0; i < vl_c2.length(); i++) {
      Serial2.write(vl_c2[i]);  // envía cada carácter individualmente
    }
    Serial2.write(",");
    Serial2.write(gear_c);
    Serial2.write(",");
    Serial2.write("E");

    /*

    Serial.print("I");
    for (int i = 0; i < rpm_c1.length(); i++) {
      Serial.print(rpm_c1[i]);  // envía cada carácter individualmente
    }
    Serial.print(",");
    for (int i = 0; i < rpm_c2.length(); i++) {
      Serial.print(rpm_c2[i]);  // envía cada carácter individualmente
    }
    Serial.print(",");
    for (int i = 0; i < vl_c1.length(); i++) {
      Serial.print(vl_c1[i]);  // envía cada carácter individualmente
    }
    Serial.print(",");
    for (int i = 0; i < vl_c2.length(); i++) {
      Serial.print(vl_c2[i]);  // envía cada carácter individualmente
    }
    Serial.print(",");
    Serial.print(gear_c);
    Serial.print(",");
    Serial.print("E");

    */

    Serial.print("Datos enviados: ");
    Serial.print("I");
    Serial.print(rpm);
    Serial.print(",");
    Serial.print(vl);
    Serial.print(",");
    Serial.print(gear);
    Serial.print(",");
    Serial.println("E");

    delay(100);
}