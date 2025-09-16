#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>

//HAN Notes - these should have more descriptive names (ones not from the example code)
//create objects for sensors and display
Adafruit_BMP280 bmp;    //BMP280 sensor for measuring the pressure.
Adafruit_AHTX0 aht;     //AHT10 sensor for measuring temperature and humidity.
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // ST7789 display object

// function for startup animation
void LoadBar(){
  tft.fillScreen(ST77XX_BLACK); //clear the screen
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_CYAN);
  //print weather station title
  tft.setCursor(20,40);
  tft.println("Weather");
  tft.setCursor(50,80);
  tft.println("Station");
  // draw a loading bar
  int barX = 20, barY = 110, barW = 200, barH=15;
  tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);
  //animate filling progress bar
  for (int i = 0; i <= barW - 2; i++) {
    tft.fillRect(barX + 1, barY + 1, i, barH - 2, ST77XX_GREEN);
    delay(10);
  }
  delay(500);  // clear the screen after animation
  tft.fillScreen(ST77XX_BLACK);
}

//HAN Notes - what is this method doing?
void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(100);
  }

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_RED); //will blank any previous text on screen
  //initialize BMP280 and AHT sensors
  if (!bmp.begin())
  {
    Serial.println("Could not find BMP280? Check wiring");
    while (1)
      ; // stop if sensor not found
  }
  Serial.println("BMP280 found");
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED); //take measurements only when requested

  if (!aht.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      ; 
  }
  Serial.println("AHT10 or AHT20 found");
  LoadBar(); //show animation on the display
}

//HAN Notes - what is this loop doing?
//HAN Notes - more comments within the method too
void loop()
{
  // take measurements from sensors
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  // Read temperature and humidity from AHT sensor
  float temperature = temp.temperature;
  float humidityVal = humidity.relative_humidity;
  float pressureVal = 0;
  // Read pressure from BMP280 sensor
  if(bmp.takeForcedMeasurement())
  {
    pressureVal = bmp.readPressure();

  }
  // Print the values to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidityVal);
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(pressureVal);
  Serial.println(" Pa");

  Serial.println();
  // Display the values on the TFT screen
  tft.setTextSize(2);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  // Clear the top part of the screen
  tft.fillRect(0,0, 240, 80, ST77XX_BLACK);
  tft.setCursor(0, 0);

  tft.println("Temperature: " + String(temperature, 1) + " C");
  tft.println("Humidity: " + String(humidityVal, 1) + " %");
  tft.println("Pressure: " + String(pressureVal / 100.0, 0) + " hPa");
  // Display status based on temperature
  tft.setTextSize(2);
  String StatusText = "";
  uint16_t textcolor = ST77XX_RED;
  uint16_t bgcolor = ST77XX_YELLOW;

  if (temperature < 10) {
    StatusText = "Warning: Low Temperature!";
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else if (temperature > 30) {
    StatusText = "Warning: High Temperature!";
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else {
    StatusText = "Temperature Normal";
    textcolor = ST77XX_GREEN;
    bgcolor = ST77XX_BLACK;
  }
  // Display the status text
  tft.fillRect(0, 90, 240, 40, bgcolor);
  tft.setCursor(0, 90);
  tft.setTextColor(textcolor, bgcolor);
  tft.println(StatusText);
  // Draw horizontal lines for separation
tft.drawLine(0, 80, 240, 80, ST77XX_WHITE);
tft.drawLine(0, 130, 240, 130, ST77XX_WHITE);

  
  delay(1000); //repeat every second
}