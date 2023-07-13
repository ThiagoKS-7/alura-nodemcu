/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
int tmpAnt = 0;
int humAnt = 0;

DHT dht(2, DHT11);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL),
// On an Esp8266  NodeMcu:   2(SDA),  1(SCL)

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  validateDisplay();

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000);

  getDHTData();  
}

void loop() {
  getDHTData(); 
}

void getDHTData(void) {
  display.clearDisplay();

  display.setTextSize(2);      // 2:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  showTempInfo();
  
  display.display();
  delay(1000);
}

void showTempInfo(void) {
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
