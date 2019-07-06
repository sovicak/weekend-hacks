// ESP8266 libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Display libraries & settings
#include <Adafruit_SSD1306.h>
#include <gfxfont.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

// Neopixel libraries + inicializing the ring on port D6
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel ring = Adafruit_NeoPixel(12, D6, NEO_GRB + NEO_KHZ800);

// Wifi settings
char ssid[] = "<replace-with-your-ssid>"; 
char pass[] = "<replace-with-your-password>"; 

void setup()
{
  Serial.begin(115200);
  delay(10);

  // Connect to Wi-Fi network
  Serial.print("Connecting to...");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println();
  Serial.println("Wi-Fi connected successfully");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize the display on address 0x3C
  display.clearDisplay();

  ring.begin();
  ring.setBrightness(50);
  ring.show();
}

// Rounding to specific number of decimal places
float roundToDp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}

// Displaying variable amount of rectangles on right side of the screen as a "loading bar" causing expected delay
// TODO Use millis() instead of delay()
void waitWithRectangles(int amount, int x_size, int y_size, int expected_delay) {
  int rectangleWait = (int) expected_delay / amount;
  
  for (int i=1; i <= amount; i++){
      delay(rectangleWait);
      display.drawRect(display.width()-5, display.height()-(y_size + 2)*i, x_size, y_size, WHITE);
      display.display();
   }
}

// Displaying single color across the whole ring
void displaySingleColor(uint32_t color) {
  int numPixels = ring.numPixels();
  for (int i=0; i < numPixels; i++){
      ring.setPixelColor(i, color);
  }
   
  ring.show();
}

// Fading in selected pixel on the ring
// TODO Remove hard-coded color, currently blue
// TODO Use millis() instead of delay()
void setPixelColorGradually(int pixel, int steps, int expected_delay) {
  int numPixels = ring.numPixels();
  for (int i=1; i <= steps; i++){
       int intensity = (int) ((255 / steps) * i);
       ring.setPixelColor(pixel, ring.Color(0, 0, intensity));
       delay((int) expected_delay / steps);
       ring.show();

   }
}

// Function accepting bearing from 0 to 360 on a LED ring with numberOfLed LEDs
void displayBearing(float bearing, int numberOfLed) {
  int pixelToShow = (int) ((bearing + (180 / numberOfLed)) / (360 / numberOfLed));
  
  // Workaround for bearings close to 360 (calculation above would overflow to non-existing pixel) 
  if(pixelToShow > numberOfLed) { pixelToShow = numberOfLed - 1; }

  // Set all LEDs to black and then fade in desired color
  displaySingleColor(ring.Color(0, 0, 0));
  setPixelColorGradually(pixelToShow, 10, 200);
}


void loop () 
{
 
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
  
      // Replace this with an URL of your Node-RED server
      http.begin("http://192.168.0.7:1880/get-closest-plane");
      
      int httpCode = http.GET();
      // Check the returning code, we will display results only when HTTP request returns 200/OK                                                                  
      if (httpCode == 200) {
        // Buffer for processing JSON
        DynamicJsonBuffer jsonBuffer;
  
        // Get the request response payload
        String payload = http.getString();
        JsonObject& plane = jsonBuffer.parseObject(payload);
  
        // Parse the JSON returned from the API into local variables
        String flight = plane["flight"];
        String distance = plane["distance"];
        String bearing = plane["bearing"];
        String distanceFormated = String(roundToDp(distance.toFloat(),2));
  
        // Display the informations about the flight
        display.clearDisplay();
        display.setTextWrap(false);
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.println(flight);
        display.println(distanceFormated + " km");
        display.display();
  
        // Show the bearing to nearest flight on the LED ring
        displayBearing(bearing.toFloat(), 12);
        
      }
      http.end();   //Close connection
    }
    
    // Waiting with graphical elements, every 500 ms we will query the API for a new information
    waitWithRectangles(8, 4, 2, 500);
}




