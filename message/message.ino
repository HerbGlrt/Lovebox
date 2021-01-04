#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <Servo.h>
#include "SSD1306Wire.h"
#include "credentials.h"
#include "images.h"

const char* ssid = _ssid;
const char* password = _password;
const String url = _url;

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LED_PIN    14
#define LED_COUNT 1

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
SSD1306Wire oled(0x3C, D2, D1);
Servo myservo;
int pos = 90;
int increment = -1;
int lightValue;
String line;
String modus;
char idSaved;
bool wasRead;


void drawMessage(const String& message) {
  oled.clear();

  // Differentiate between text and image
  if (modus[0] == 't') {
    oled.drawStringMaxWidth(0, 15, 128, message);
  }
  else {
    for (int i = 0; i <= message.length(); i++) {
      int x = i % 129;
      int y = i / 129;

      if (message[i] == '1') {
        oled.setPixel(x, y);
      }
    }
  }
  oled.display();
}

void wifiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }
}

void getGistMessage() {
  const int httpsPort = 443;
  const char* host = "gist.githubusercontent.com";
  const char fingerprint[] = "70 94 DE DD E6 C4 69 48 3A 92 70 A1 48 56 78 2D 18 64 E0 B7";

  WiFiClientSecure client;
  client.setFingerprint(fingerprint);
  if (!client.connect(host, httpsPort)) {
    return; // Connection Failed
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String temp = client.readStringUntil('\n');
    if (temp == "\r") {
      break;
    }
  }
  String id = client.readStringUntil('\n');
  if (id[0] != idSaved) { // New message
    wasRead = 0;
    idSaved = id[0];
    EEPROM.write(142, idSaved);
    EEPROM.write(144, wasRead);
    EEPROM.commit();

    modus = client.readStringUntil('\n');
    line = client.readStringUntil(0);
    drawMessage(line);
  }
}

void rainbow(int wait) {
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    myservo.write(pos);
    if (pos == 75 || pos == 105) {
      increment *= -1;
    }
    pos += increment;
    if (analogRead(0) > 300) {
      break;
    }
    delay(40);
  }
  myservo.write(90);
  strip.clear();
  strip.show(); // When the cycle of the heart's spin is complete, the LEDs switch to the next color
}


void setup() {
  myservo.attach(16);   // Servo an D0

  oled.init();
  oled.flipScreenVertically();
  oled.setColor(WHITE);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
 oled.setFont(ArialMT_Plain_10);


  strip.begin();              // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();              // Turn OFF all pixels ASAP
  strip.setBrightness(220); // Set BRIGHTNESS to about 220 (max = 255)
  strip.clear();
  strip.show();

  wifiConnect();

  EEPROM.begin(512);
  idSaved = EEPROM.get(142, idSaved);
  wasRead = EEPROM.get(144, wasRead);

  oled.clear();
  oled.drawXbm(38, 16, CatLogo_width, CatLogo_height, Cat);
  oled.drawString(7, 0, "<3 LoveBox <3");
  oled.display(); // Displaying default message and image
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }

  if (wasRead) {
    getGistMessage();
  }

if (analogRead(0) < 300 & wasRead == 1){
  oled.clear();
  oled.drawXbm(38, 16, CatLogo_width, CatLogo_height, Cat);
  oled.drawString(7, 0, "<3 LoveBox <3");
  oled.display();// After the message has been read and the lid is closed, the display returns to it's defalt state
}

  while (!wasRead) {
    rainbow(10);     // Rotate heart and start rainbow effect
    lightValue = analogRead(0);      // Read brightness value
    if (lightValue > 300) {
      wasRead = 1;
      EEPROM.write(144, wasRead);
      EEPROM.commit();
    }
  }
}
