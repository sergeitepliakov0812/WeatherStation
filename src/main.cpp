#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>


//create objects for sensors and display
Adafruit_BMP280 bmpsens;    //BMP280 sensor for measuring the pressure.
Adafruit_AHTX0 ahtsens;     //AHT10 sensor for measuring temperature and humidity.
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // ST7789 display object

//GRAPH FUNCTION
void Graph(float value, int graphY, int graphHeight, float minVal, float maxVal, uint16_t color) {
static int xPos = 0; //current x position 
static float lastTempVal, lastHumidVal, lastPressVal; //last values for drawing lines

//decide which value to graph
float *lastVal;
if (graphY == 0) lastVal = &lastTempVal; //temperature graph
else if (graphY == 50) lastVal = &lastHumidVal; //humidity graph
else lastVal = &lastPressVal; //pressure graph

//scale sensor value to fit graph height
int scaledVal = map(value, minVal, maxVal, graphHeight, 0);

//draw line from last value to current value
if (xPos > 0) { //skip drawing line for first point
  tft.drawLine(xPos - 1, graphY + *lastVal, xPos, graphY + scaledVal, color);
}
*lastVal = scaledVal; //update last value
// move forward x position
xPos++;

//reset graph when reaching the end
if (xPos >= 240) {
  xPos = 0;   
  *lastVal = -1; //reset last value to avoid drawing line across screen
  tft.fillRect(0, graphY, 240, graphHeight, ST77XX_BLACK); //clear graph area
}
}
// function for startup animation
//shows a startup loading bar animation
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
  int barX = 20, barY = 110, barW = 200, barH=15; //position and size of the bar
  tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);  //draw border
  //animate filling progress bar
  for (int i = 0; i <= barW - 2; i++) { 
    tft.fillRect(barX + 1, barY + 1, i, barH - 2, ST77XX_GREEN); //fill bar
    delay(10);
  }
  delay(500);  // clear the screen after animation
  tft.fillScreen(ST77XX_BLACK);
}

//Method runs once at startup
void setup()
{
  Serial.begin(115200); //starts serial communication at 115200 baud rate
  while (!Serial) // wait for serial port to connect. Needed for native USB
  {
    delay(100);
  }

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT + I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT display
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_RED); //temporary red screen to show its working
  //initialize BMP280 and AHT sensors
  if (!bmpsens.begin())
  {
    Serial.println("Could not find BMP280? Check wiring");
    while (1)
      ; // stop if sensor not found
  }
  Serial.println("BMP280 found");
  bmpsens.setSampling(Adafruit_BMP280::MODE_FORCED); //take measurements only when requested

  if (!ahtsens.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      ; 
  }
  Serial.println("AHT10 or AHT20 found");
  LoadBar(); //show animation on the display
}


//run repeatedly after setup() is done
void loop()
{
  // take measurements from sensors
  sensors_event_t humidity, temp;
  ahtsens.getEvent(&humidity, &temp);
  // Read temperature and humidity from AHT sensor
  float temperature = temp.temperature;
  float humidityVal = humidity.relative_humidity;
  float pressureVal = 0;
  // Read pressure from BMP280 sensor
  if(bmpsens.takeForcedMeasurement()) // Request a measurement when needed
  {
    pressureVal = bmpsens.readPressure(); 

  }
  // Print the values to Serial Monitor
  Serial.print("Temperature: ");// Print temperature in Celsius
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");// Print relative humidity
  Serial.print(humidityVal);//
  Serial.println(" %");

  Serial.print("Pressure: ");// Print pressure in Pascals
  Serial.print(pressureVal);
  Serial.println(" Pa");

  Serial.println();
  // Display the values on the TFT screen
  tft.setTextSize(2);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);// Set text color to magenta with black background
  // Clear the top part of the screen
  tft.fillRect(0,0, 240, 80, ST77XX_BLACK);// Clear area for new data
  tft.setCursor(0, 0);// Set cursor to top-left corner
// Print temperature, humidity, and pressure
  tft.println("Temperature: " + String(temperature, 1) + " C");
  tft.println("Humidity: " + String(humidityVal, 1) + " %");
  tft.println("Pressure: " + String(pressureVal / 100.0, 0) + " hPa");
  // Display status based on temperature
  tft.setTextSize(2);
  String StatusText = "";// Variable to hold status message
  uint16_t textcolor = ST77XX_RED;// Default text color
  uint16_t bgcolor = ST77XX_YELLOW;// Default background color
// Determine status based on temperature thresholds
  if (temperature < 10) {
    StatusText = "Warning: Low Temperature!"; //displays warning if temp is below 10C
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else if (temperature > 30) { 
    StatusText = "Warning: High Temperature!"; //displays warning if temp is above 30C
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else {
    StatusText = "Temperature Normal"; //displays normal if temp is between 10C and 30C
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

//Graph the sensor values
Graph(temperature, 0, 50, -10, 40, ST77XX_RED); //graph temperature in top section
Graph(humidityVal, 50, 50, 0, 100, ST77XX_BLUE); //graph humidity in middle section
Graph(pressureVal, 100, 35, 950, 1050, ST77XX_GREEN); //graph pressure in bottom section
  
  delay(1000); //repeat every second
}