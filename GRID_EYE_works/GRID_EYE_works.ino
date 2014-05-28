/*
GRID-EYE 8x8 thermal camera board
uses http://www.adafruit.com/products/938
or http://www.adafruit.com/products/326 as a screen

NOTE, needs modified SSD1306 driver to work
go here to get it: https://github.com/AKAMEDIASYSTEM/Adafruit_SSD1306_rfMOD

main processor is an rfd22310

see https://github.com/AKAMEDIASYSTEM/grideye_rfduino for more info



MIT License

*/
#include <SPI.h>
#include <Adafruit_SSD1306ms.h>
#include <Adafruit_GFX.h>

#include <RFduinoBLE.h>
#include <Wire.h>

#define OLED_RESET 4
Adafruit_SSD1306 scr(OLED_RESET);

float pixels[64];
float transposed[64];
byte pixelTempL;
byte pixelTempH;
char addr = 0x68;
float celsius;
float celsiusTherm;
float minCel = 900.0;
float maxCel = -900.0;
char buf[10];
bool isConn = false;

// note, holy shit this seems to work as of april 14 2014

void setup() {
  RFduinoBLE.deviceName = "grideye";
  RFduinoBLE.advertisementData = "AKA";
  RFduinoBLE.advertisementInterval = MILLISECONDS(300);
  RFduinoBLE.txPowerLevel = -20;  // (-20dbM to +4 dBm)

  // start the BLE stack
  RFduinoBLE.begin();

  Wire.speed = 400;
  Wire.begin();
  //  Serial.begin(9600);
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  delay(10);
  scr.begin(SSD1306_SWITCHCAPVCC, 0x3D); //changed to 0x3C to use the 4-wire OLEDs I got from Tindie.
  //  Adafruit OLEDs either have an address pin (left unconnected, default address seems to be 0x3D) or a label on the pcb
  scr.clearDisplay();
  scr.setTextColor(WHITE);
  scr.setTextSize(2);
  scr.setCursor(12, 12);
  scr.println("AKA");
  scr.setTextSize(1);
  scr.println("GRID-EYE v2");
  scr.display();
  delay(1000);

}
void loop()
{
  // switch to lower power mode
  //  RFduino_ULPDelay(INFINITE);
  // to send one char
  // RFduinoBLE.send((char)temp);

  // to send multiple chars
  // RFduinoBLE.send(&data, len);

  scr.clearDisplay();
  digitalWrite(3, LOW);
  //First two data registers for the pixel temp data are 0x80 and 0x81
  pixelTempL = 0x80;
  pixelTempH = 0x81;
  maxCel = -900;
  minCel = 900;
  //Get Temperature Data for each pixel in the 8x8 array. Will loop 64 times.
  for (int pixel = 0; pixel <= 63; pixel++) {
    //Get lower level pixel temp byte
    Wire.beginTransmission(addr);
    Wire.write(pixelTempL);
    Wire.endTransmission();
    Wire.requestFrom(addr, 1);
    byte lowerLevel = Wire.read(); //
    Wire.endTransmission();

    Wire.beginTransmission(addr);
    Wire.write(pixelTempH);
    Wire.endTransmission();
    Wire.requestFrom(addr, 1);
    byte upperLevel = Wire.read();
    //Combine the two bytes together to complete the 12-bit temp reading
    int temperature = ((upperLevel << 8) | lowerLevel);
    //Temperature data is in two's compliment, do conversion.
    if (upperLevel != 0)
    {
      temperature = -(2048 - temperature);
    }
    celsius = temperature * 0.25;
    pixels[pixel] = celsius;
    if (celsius > maxCel) {
      maxCel = celsius;
    }
    if (celsius < minCel) {
      minCel = celsius;
    }
    //Go to next pixel by advancing both the low and high bit two register values
    pixelTempL = pixelTempL + 2;
    pixelTempH = pixelTempH + 2;
  }

  // done reading this frame
  digitalWrite(3, HIGH);

  //Thermistor Register - Optional
  Wire.beginTransmission(addr);
  Wire.write(0x0E);
  Wire.endTransmission();
  Wire.requestFrom(addr, 1);
  byte upperLevelTherm = Wire.read();
  Wire.beginTransmission(addr);
  Wire.write(0x0F);
  Wire.endTransmission();
  Wire.requestFrom(addr, 1);
  byte lowerLevelTherm = Wire.read();
  int temperatureTherm = ((lowerLevelTherm << 8) | upperLevelTherm);
  celsiusTherm = temperatureTherm * 0.0625;

  // display stuff

  displayTemperaturesR();
  displaySpecs();
  sendFrame();
  scr.display();


} //end loop

void sendFrame() {
  for (int fr = 0; fr < 8; fr++) {

    buf[0] = fr;
    buf[1] = char(celsiusTherm);
    for (int i = 0; i < 8; i++) {
      buf[2 + i] = char(pixels[(fr * 8) + i]);
    }
    RFduinoBLE.send(buf, 10);
  }
}

void displayTemperaturesR() { // rotated for grideye-toy

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int pixel = (7 - row) + (col % 8) * 8;
      float t = pixels[pixel] - celsiusTherm;
      float s = 0.5 * map(t, minCel - celsiusTherm, maxCel - celsiusTherm, 0.0, 10.0); // map from 0C to 40 deg C
      int xp = (8 - row) * 8 + 24; // 24 is to center grid on 128x64 screen
      int yp = col * 8;
      scr.fillRect(xp, yp, s, s, WHITE);
    }
  }
}

void displaySpecs() {
  scr.setTextSize(1);
  scr.setCursor(108, 44);
  scr.print("Bkg");
  scr.setCursor(108, 54);
  scr.print((int)celsiusTherm);
  scr.setCursor(0, 0);
  scr.print("Min");
  scr.setCursor(0, 10);
  scr.print(minCel);
  scr.setCursor(0, 54);
  scr.print(maxCel);
  scr.setCursor(0, 44);
  scr.print("Max");

  //image borders
  scr.drawLine(31, 0, 31, 64, WHITE);
  scr.drawLine(95, 0, 95, 64, WHITE);

  // connected flag
  if (isConn) {
    scr.fillTriangle(108, 0, 127, 0, 118, 10, WHITE);
  } else {
    scr.fillTriangle(108, 0, 127, 0, 118, 10, BLACK);
  }
}


// unused, but for lib reference
void displayTemperatures() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int t = pixels[col + (8 * row)];
      //      int t = random(15, 40);
      float s = 0.5 * constrain(map(t - celsiusTherm, minCel - celsiusTherm, maxCel - celsiusTherm, 0.0, 10.0), 0.0, 10.0); // map from 0C to 40 deg C
      //      Serial.println(s / 100.0);
      scr.fillRect(row * 8 + 4 + 32, col * 8 + 4, s, s, WHITE);
    }
  }
}
