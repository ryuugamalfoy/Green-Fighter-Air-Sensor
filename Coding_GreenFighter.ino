#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include "DHT.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi and ThingSpeak settings
#define SECRET_SSID "Alhamdulillah 2.4Ghz"
#define SECRET_PASS "malaysia12"
#define SECRET_CH_ID 2298183
#define SECRET_WRITE_APIKEY "EKLA4IUW82XQBZ2Y"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;
WiFiClient client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;
String myStatus = "";

// Pin definitions
#define MQ135PIN 33
SoftwareSerial pmsSerial(16, 17);

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

#define SENSOR_IN 0 // Update your PIN
#define DHTPIN 32
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
int pushButton = 2;



void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println("The Green Fighter!");

  delay(1000);

  pmsSerial.begin(9600);

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  //dht 11
  dht.begin();
  pinMode(pushButton, INPUT);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("The Green Fighter!");
  display.display();
  delay(100);

Serial.begin(115200); // Initialize serial communication
}

boolean readPMSdata(Stream *s) {
  if (!s->available()) {
    return false;
  }

  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  return true;
}

void loop() {

  static int displayCounter = 0;

  if (readPMSdata(&pmsSerial)) {
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (standard)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (environmental)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
    Serial.println("---------------------------------------");
    Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
    Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
    Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
    Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
    Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
    Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
    Serial.println("---------------------------------------");

    // Read MQ135 data (CO2)
    int CO2Value = analogRead(MQ135PIN);
    Serial.print("CO2 Value: ");
    Serial.print(CO2Value);
    Serial.println(" ppm");  // Parts per million

    // Read CO sensor data
    int coSensorValue = analogRead(MQ135PIN);
    float coVoltage = coSensorValue * (1.49 / 1023.0);
    // You need to adjust the conversion equation based on your CO sensor's datasheet
    // The following is just an example
    float coPPM = 10 * ((coVoltage - 0.22) / 0.8);
    Serial.print("CO Concentration: ");
    Serial.print(coPPM);
    Serial.println(" ppm");

    //dht11
    // Read sensor data
  delay(5000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("%");

    // Update OLED display based on the displayCounter
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("The Green Fighter!");

    switch (displayCounter) {

      case 0:
        display.setCursor(0, 12);
        display.println("PM 1: ");
        display.print(data.pm10_standard);
        display.println(" ppm");
        break;

      case 1:
        display.setCursor(0, 12);
        display.println("PM 10: ");
        display.print(data.pm100_standard);
        display.println(" ppm³");
        break;

      case 2:
        display.setCursor(0, 12);
        display.print("PM2.5: ");
        display.print(data.pm25_standard);
        display.println(" ppm");
        break;
      
      case 3:
        display.setCursor(0, 12);
        display.print("humidity: ");
        display.print(h);
        display.println(" %");
        break;

      case 4:
        display.setCursor(0, 12);
        display.print("CO2: ");
        display.print(CO2Value);
        display.println(" ppm");
        break;
      
      case 5:
        display.setCursor(0, 12);
        display.print("Temperature: ");
        display.print(t);
        display.println(" C");
        break;
    }

    display.display();

    // Increment the counter for the next iteration
    displayCounter = (displayCounter + 1) % 6;

    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);
        Serial.print(".");
        delay(5000);
      }
      Serial.println("\nConnected.");
    }

    // Set the fields with the values
    int tempValue = 0; // Replace this with actual temperature value if available
    int humidityValue = 0; // Replace this with actual humidity value if available

    // Set the fields with the values
    ThingSpeak.setField(1, data.pm10_standard);
    ThingSpeak.setField(2, data.pm25_standard);
    ThingSpeak.setField(3, data.pm100_standard);
    ThingSpeak.setField(4, CO2Value);
    ThingSpeak.setField(5, coPPM);
    ThingSpeak.setField(6, t);
    ThingSpeak.setField(7, h);


    // Set the status
    ThingSpeak.setStatus(myStatus);

    // Write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    delay(15000);
  }
}
