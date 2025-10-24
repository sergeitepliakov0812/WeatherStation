// ...existing code...
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>

#define GRAPH_WIDTH 240
#define GRAPH_HEIGHT 50
#define GRAPH_X 0


//create objects for sensors and display
Adafruit_BMP280 bmpsens;    // BMP280 sensor object (pressure)
Adafruit_AHTX0 ahtsens;     // AHT10/AHT20 sensor object (temp & humidity)
// ST7789 display object — constructor expects chip-select, dc, reset pins defined elsewhere
Adafruit_ST7789 tftScreen = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // ST7789 display object

//GRAPH FUNCTION
// Graph draws a moving line plot for a single value band.
// value      - sensor reading to plot
// graphY     - vertical offset on screen where this graph's top starts
// graphHeight- pixel height available for this graph
// minVal/maxVal - expected data range to scale into pixels
// color      - color used for the line
void Graph(float value, int graphY, int graphHeight, float minVal, float maxVal, uint16_t color) {
  static int xPos = 0; // current x position (advances each call)
  // last values for drawing lines; initialized to -1 to indicate "no previous point"
  static float lastTempVal = -1, lastHumidVal = -1, lastPressVal = -1;

  //decide which value to graph (select pointer to the appropriate last-value variable)
  float *lastVal;
  if (graphY == 0) lastVal = &lastTempVal; // temperature graph (top band)
  else if (graphY == 50) lastVal = &lastHumidVal; // humidity graph (middle band)
  else lastVal = &lastPressVal; // pressure graph (bottom band)

  //scale sensor value to fit graph height
  // map() works with integers; using floats here relies on implicit conversion
  int scaledVal = map(value, minVal, maxVal, graphHeight, 0);

  //draw line from last value to current value (skip first point)
  if (*lastVal >= 0) { // if we have a previous point, draw connecting line
    tftScreen.drawLine(xPos - 1, graphY + *lastVal, xPos, graphY + scaledVal, color);
  }
  *lastVal = scaledVal; // update last value for next call

  // move forward x position for next sample
  xPos++;

  //reset graph when reaching the end of the display width
  if (xPos >= 240) {
    xPos = 0;
    *lastVal = -1; // reset last value to avoid drawing a wrap-around line
    // clear graph area so new plot starts on blank background
    tftScreen.fillRect(0, graphY, 240, graphHeight, ST77XX_BLACK);
  }
}
// function for startup animation
// shows a startup loading bar animation and a title
void LoadBar(){
  tftScreen.fillScreen(ST77XX_BLACK); // clear the screen
  tftScreen.setTextSize(3);           // larger text for title
  tftScreen.setTextColor(ST77XX_CYAN);
  // print weather station title
  tftScreen.setCursor(20,40); // position for title
  tftScreen.println("Weather");
  tftScreen.setCursor(50,80); // position for subtitle
  tftScreen.println("Station");
  // draw a loading bar
  int barX = 20, barY = 110, barW = 200, barH=15; // position and size of the bar
  tftScreen.drawRect(barX, barY, barW, barH, ST77XX_WHITE);  // draw border
  // animate filling progress bar
  for (int i = 0; i <= barW - 2; i++) {
    tftScreen.fillRect(barX + 1, barY + 1, i, barH - 2, ST77XX_GREEN); // fill bar
    delay(10); // simple timing for animation
  }
  delay(500);  // pause before clearing the screen
  tftScreen.fillScreen(ST77XX_BLACK);
}

//Method runs once at startup
void setup()
{
  Serial.begin(115200); // starts serial communication at 115200 baud rate
  while (!Serial) // wait for serial port to connect. Needed for native USB
  {
    delay(100);
  }

  // turn on backlight pin (assumes TFT_BACKLITE defined)
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT + I2C power supply (assumes TFT_I2C_POWER defined)
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT display
  // tftScreen.init(width, height) — width/height arguments are driver-dependent
  tftScreen.init(135, 240); // Init ST7789 240x135
  tftScreen.setRotation(3); // set screen rotation to match wiring/orientation
  tftScreen.fillScreen(ST77XX_RED); // temporary red screen to show it's working

  // initialize BMP280 sensor
  if (!bmpsens.begin())
  {
    Serial.println("Could not find BMP280? Check wiring");
    while (1)
      ; // stop if sensor not found — prevents proceeding with missing hardware
  }
  Serial.println("BMP280 found");
  // use forced mode so measurements occur only when requested
  bmpsens.setSampling(Adafruit_BMP280::MODE_FORCED);

  // initialize AHT sensor
  if (!ahtsens.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      ;
  }
  Serial.println("AHT10 or AHT20 found");
  LoadBar(); // show animation on the display
}


//run repeatedly after setup() is done
void loop()
{
  // take measurements from sensors
  sensors_event_t humidity, temp;
  ahtsens.getEvent(&humidity, &temp); // fills humidity and temp events

  // Read temperature and humidity from AHT sensor
  float temperature = temp.temperature;            // degrees Celsius
  float humidityVal = humidity.relative_humidity;  // percent rH
  float pressureVal = 0;                            // will hold pressure in Pa

  // Read pressure from BMP280 sensor (forced measurement)
  if(bmpsens.takeForcedMeasurement()) // Request a measurement when needed
  {
    pressureVal = bmpsens.readPressure(); // read pressure in Pascals
  }

  // Print the values to Serial Monitor for debugging/logging
  Serial.print("Temperature: "); // Print temperature in Celsius
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: "); // Print relative humidity
  Serial.print(humidityVal);
  Serial.println(" %");

  Serial.print("Pressure: "); // Print pressure in Pascals
  Serial.print(pressureVal);
  Serial.println(" Pa");

  Serial.println();

  // Display the values on the TFT screen
  tftScreen.setTextSize(2); // set text scaling (integer values only)
  tftScreen.setTextWrap(false); // disable automatic wrapping of long text
  // Use magenta text on a black background for the numeric area
  tftScreen.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);

  // Clear the top part of the screen where numeric data is shown
  tftScreen.fillRect(0,0, 240, 80, ST77XX_BLACK); // clear area for new data
  tftScreen.setCursor(0, 0); // Set cursor to top-left corner for printing

  // Print temperature, humidity, and pressure lines
  // Using String() for simple formatting (watch heap fragmentation on small MCUs)
  tftScreen.println("Temperature: " + String(temperature, 1) + " C");
  tftScreen.println("Humidity: " + String(humidityVal, 1) + " %");
  // pressureVal is in Pa; convert to hPa for more common meteorological units
  tftScreen.println("Pressure: " + String(pressureVal / 100.0, 0) + " hPa");

  // Display status based on temperature thresholds
  tftScreen.setTextSize(2); // ensure status text uses same scale
  String StatusText = ""; // Variable to hold status message
  uint16_t textcolor = ST77XX_RED; // Default text color (used for warnings)
  uint16_t bgcolor = ST77XX_YELLOW; // Default background color for warnings

  // Determine status based on temperature thresholds
  if (temperature < 10) {
    StatusText = "Warning: Low Temperature!"; // low temp warning
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else if (temperature > 30) {
    StatusText = "Warning: High Temperature!"; // high temp warning
    textcolor = ST77XX_RED;
    bgcolor = ST77XX_YELLOW;
  } else {
    StatusText = "Temperature Normal"; // nominal temperature range
    textcolor = ST77XX_GREEN;
    bgcolor = ST77XX_BLACK;
  }

  // Display the status text within a filled background rectangle
  tftScreen.fillRect(0, 90, 240, 40, bgcolor); // background for status line
  tftScreen.setCursor(0, 90); // position cursor at the status area
  tftScreen.setTextColor(textcolor, bgcolor); // set text & background colors
  tftScreen.println(StatusText);

  // Draw horizontal separation lines between sections
  tftScreen.drawLine(0, 80, 240, 80, ST77XX_WHITE);
  tftScreen.drawLine(0, 130, 240, 130, ST77XX_WHITE);

  // Graph the sensor values in three vertical bands (top, middle, bottom)
  Graph(temperature, 0, 50, -10, 40, ST77XX_RED);    // graph temperature in top section
  Graph(humidityVal, 50, 50, 0, 100, ST77XX_BLUE);   // graph humidity in middle section
  Graph(pressureVal, 100, 35, 950, 1050, ST77XX_GREEN); // graph pressure in bottom section (hPa range)

  delay(1000); // repeat every second (basic timing; blocks execution)
}