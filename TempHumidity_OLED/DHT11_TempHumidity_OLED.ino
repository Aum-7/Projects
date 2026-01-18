#include <GyverOLED.h>   // Library for controlling the OLED display (GyverOLED by AlexGyver)
#include <DHT.h>         // Library for DHT temperature/humidity sensors (DHT sensor library by Adafruit)

GyverOLED<SSH1106_128x64> oled;  // Create OLED object for 128x64 SSH1106 display

#define DHTPIN 8         // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11    // Sensor type: DHT11
DHT dht(DHTPIN, DHTTYPE); // Create DHT sensor object



void setup() {
  Serial.begin(9600);    // Start serial communication for debugging
  dht.begin();           // Initialize the DHT sensor
  oled.init();           // Initialize the OLED display
  oled.clear();          // Clear the display buffer
}


void loop() {
  float t = dht.readTemperature(); // Read temperature in Celsius
  float h = dht.readHumidity();    // Read humidity percentage
  
  // Check if sensor returned valid data
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  oled.setScale(1);      // Set text size
  oled.setCursor(0, 1);  // Position cursor for temperature display

  oled.print("Temperature: ");  // Indicate measurement

  // Print temperature in Fahrenheit
  
  oled.print((t * (9/5) + 32));  // Convert Celsius to Fahrenheit
  oled.print(" F");

  // Uncomment this block to print temperature in Celsius instead
  /*
  oled.print(t);         // Print temperature in Celsius
  oled.print(" C");
  */

  oled.update();         // Refresh the display to show new content
  delay(1000);           // Wait 1 second

  oled.setScale(1);      // Set text size again (optional but consistent)
  oled.setCursor(0, 3);  // Position cursor for humidity display

  oled.print("Humidity: ");  // Indicate measurement

  oled.print(h);         // Print humidity value
  oled.print(" %");      // Add percent symbol


  oled.update();         // Refresh display
  delay(1000);           // Wait 1 second
}
