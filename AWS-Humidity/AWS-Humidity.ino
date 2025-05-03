#include "Secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "WiFi.h"
#include "DHT.h"
 
#define DHTPIN 14   // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 11
#define ERROR 13
#define SUCCESS 27
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 

#define SCL_PIN 33
#define SDA_PIN 25

float h ;
float t;
 
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
 
void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
  Serial.println(AWS_IOT_ENDPOINT);
  Serial.println(THINGNAME);
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print("x");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    digitalWrite(ERROR, HIGH);
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  digitalWrite(SUCCESS, HIGH);
  display.println(F("AWS IoT Connected!"));
  delay(100);
  Serial.println("AWS IoT Connected!");

}
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humidity"] = h;
  doc["temperature"] = t;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void blinkLED() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(SUCCESS, LOW);
    delay(100);
    digitalWrite(SUCCESS, HIGH);
    delay(100);
  }
}
 
void setup()
{

  Wire.begin(SDA_PIN, SCL_PIN);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  Serial.println(F("SSD1306 allocation failed"));
  for(;;);
  }

  pinMode(ERROR, OUTPUT);
  pinMode(SUCCESS, OUTPUT);
  Serial.begin(115200);
  connectAWS();
  dht.begin();

display.display();
delay(2000);
display.clearDisplay();
display.setTextSize(2);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 10);
display.println(F("Hola, Mundo!"));
display.display();

}
 
void loop()
{
  digitalWrite(SUCCESS, HIGH);
  h = dht.readHumidity();
  t = dht.readTemperature();
 
 
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    digitalWrite(ERROR, HIGH);
    delay(2000);
    return;
  }
 
  display.clearDisplay();
  Serial.print(F("Humidity: "));
  Serial.print(h);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print(F("Hum:"));
  display.print(h);
  display.println("%");
  display.println();
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  display.print(F("Tmp:"));
  display.print(t);
  display.print("C");
  display.display();
 
  publishMessage();
  blinkLED();
  client.loop();
  delay(300000);
  //delay(2000);
}
