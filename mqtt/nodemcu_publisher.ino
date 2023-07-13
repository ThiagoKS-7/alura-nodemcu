#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*ACKNOLEDGEMENT*/
//main reference: https://randomnerdtutorials.com/esp8266-nodemcu-mqtt-publish-bme680-arduino/


// -- DISPLAY OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL),
// On an Esp8266  NodeMcu:   2(SDA),  1(SCL)

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);




// DHT11 SENSOR

DHT dht(2, DHT11);
int tmpAnt = 0; // anterior temperature, so we can save the previous value, to show on display
int humAnt = 0; // anterior humidity, so we can save the previous value, to show on display




// -------- WIFI  -------- //
#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

// Raspberry Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, XXX)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "nodemcu/dht11/temperature"
#define MQTT_PUB_HUM  "nodemcu/dht11/humidity"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings
// -------- WIFI  -------- //



void setup() {
  Serial.begin(115200);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  connectToWifi();


  validateDisplay(); // Tries to find display on the given address
  display.display(); // Shows splash screen
  delay(1000);

  getDHTData();  
}

void loop() {
  getDHTData(); 
}

// -------- WIFI  -------- //

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void publishTempOnBroker(void) {
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // Publish an MQTT message on topic nodemcu/dht11/temperature, at QoS 1
    uint16_t packetIdPubTemp = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temperature).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPubTemp);
    Serial.printf("Message: %.2f \n", temperature); 
  }
}


void publishHumidityOnBroker(void) {
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // Publish an MQTT message on topic nodemcu/dht11/humidity, at QoS 1
    uint16_t packetIdPubHum = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(humidity).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPubHum);
    Serial.printf("Message: %.2f \n", humidity);
  }
}

// -------- WIFI -------- //


// -------- DHT11 -------- //

void getDHTData(void) {
  display.clearDisplay();

  display.setTextSize(2);      // 2:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  showTempInfo();
  
  display.display();
  delay(1000);
}

// -------- DHT11 -------- //


// -------- DISPLAY -------- //

void showTempInfo(void) {
  unsigned long currentMillis = millis();
  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature(false);

  display.setCursor(0, 0);     // Start at mid-left corner
  display.write("Temp:");
  if (temperature > 999 || isnan(temperature)) {
    display.print(tmpAnt);
    tmpAnt = 0;
  } else {
    display.print(temperature);
    tmpAnt = temperature;
    publishTempOnBroker();
  }
  display.write(" C");

  display.setCursor(0, 18);
  display.write("Hum:");
  if (humidity > 999 || isnan(humidity)) {
    display.print(humAnt);
    humAnt = 0;
  } else {
    display.print(humidity);
    humAnt = humidity;
    publishHumidityOnBroker();
  }
  display.write(" %");
}

void validateDisplay(void){
   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

}
// -------- DISPLAY -------- //
