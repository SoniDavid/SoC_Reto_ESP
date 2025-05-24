#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

SoftwareSerial espSerial(4, 0);

extern "C" { 
#include "EngTrModel.h" 
}

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
      // client.subscribe("esp32/control");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  EngTrModel_initialize();
  //setup_wifi();
  //client.setServer(mqtt_server, mqtt_port);
}


void loop() {
  //if (!client.connected()) {
  //  reconnect();
  //}

  //client.loop();

  long now = millis();

  if (now - lastMsg > msgInterval) {
    lastMsg = now;
    EngTrModel_U.Throttle = 50.0;
    EngTrModel_U.BrakeTorque = 0.0;
    EngTrModel_step( );

    float rpm = EngTrModel_Y.VehicleSpeed;
    float vl = EngTrModel_Y.EngineSpeed;
    float gear = EngTrModel_Y.Gear;

    int rpm_i = (int)(rpm*100);
    int vl_i = (int)(vl*100);
    int gear_i = (int)(gear);
    

    String line_re = "I"+  String(rpm_i / 100) + ","  + String(rpm_i % 100) + "," + String(vl_i / 100)+ "," + String(vl_i % 100) + "," + String(gear_i) + ",E";
    espSerial.println(line_re);
    Serial.println("Datos enviados: " + line_re);

    line_re = "I"+  String(rpm) + "," + String(vl) + "," + String(gear) + ",";
    //Serial.println("Datos enviados: " + line_re);

    float rpm_ = 0;
    float vl_ = 0;
    float gear_ = 0;

    delay(50);

    if(espSerial.available())
    {
      String line = espSerial.readStringUntil('\n');
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
}