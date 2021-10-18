//This project was based off of Volos Projects: https://www.youtube.com/channel/UCit2rVgOvhyuAD-VH5H_IHg
//and
//That Project: https://www.youtube.com/channel/UCRr2LnXXXuHn4z0rBvpfG7w
//Huge thank you to these guys!

//The display library files have to be modified and are added along with this sketch

#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_SGP30.h"
#include "ClosedCube_HDC1080.h"
#include "Adafruit_VL53L0X.h"
#include "icons.h"
#include <TJpg_Decoder.h>  //The jpeg decoder library
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson.git
#include <NTPClient.h>           //https://github.com/taranais/NTPClient
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson.git

#define TFT_BACKLIGHT  26

BH1750 lightMeter;
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
ClosedCube_HDC1080 hdc1080;
Adafruit_SGP30 sgp;
Adafruit_NeoPixel pixels(1, 5, NEO_GRB + NEO_KHZ800);

#include "Orbitron_Medium_20.h"
#define TFT_GREY 0x5AEB
#define TFT_lightblue 0x01E9
#define TFT_darkred 0xA041
#define TFT_blue 0x5D9B
#define TFT_aqua 0x04FF
#define TFT_yellow 0xFFE0
#define TFT_olive 0x7BE0
#define TFT_purple 0x780F
#define TFT_maroon 0x7800
#define TFT_navy 0x000F
#define TFT_silver 0xA510
#define TFT_brown 0x8200
#define TFT_violet 0x9199
#define TFT_red 0xF800
#define TFT_lime 0x87E0
#define TFT_light_grey 0xC618

//Buzzer
const int ledPin = 12;  //Buzzer
const int buzzer_freq = 5000;
const int buzzer_ledChannel = 1;
const int buzzer_resolution = 8;

byte omm = 99;
bool initial = 1;
byte xcolon = 0;
unsigned int colour = 0;

//Poll Weather interval
const long interval = 60000; 
unsigned long previousMillis = 0;    //will store last time weather conditions were updated    

//PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8; 

uint32_t targetTime = 0;       // for next 1 second timeout

//CO2
uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
 // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
 const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
 const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
 return absoluteHumidityScaled;
}

//Replace with your network credentials
const char *ssid     = "SSID";        
const char *password = "PASSWORD";  

//Open Weather Settings
String town="Hantsport";            //EDDIT
String Country="CA";                //EDDIT
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
const String key = "API KEY"; /*Open Weather API Key*/
String formattedDate;
String dayStamp;

String payload=""; //whole json 
String tmp="" ; //temperature
String hum="" ; //humidity
String weather="" ; //weather
String description="" ; //description
String tempmin="" ; //temperature
String tempmax="" ; //temperature
int weatherID = 0;  
int id;  

StaticJsonDocument<1000> doc;

//Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if ( y >= tft.height() ) return 0; //Stop further decoding as image is running off bottom of screen
  tft.pushImage(x, y, w, h, bitmap);  //clips the image block rendering automatically at the TFT boundaries
  return 1;    //Return 1 to decode next block
}

void Dim_Display()
{
 ledcWrite(ledChannel, 20);  //DIM 
 for(int dutyCycle = 255; dutyCycle >= 10; dutyCycle--)
 {
  ledcWrite(ledChannel, dutyCycle);  //decrease the LED brightness
  delay(15);
 }
} 

void Brighten_Display()
{
 for(int dutyCycle = 10; dutyCycle <= 255; dutyCycle++)
 {   
  ledcWrite(ledChannel, dutyCycle); //increase the LED brightness
  delay(15);
 }
} 

void setup() 
{
 Serial.begin(115200);
 Wire.begin(13,14);
 delay(150);
  
 ledcSetup(ledChannel, buzzer_freq, resolution);  //PWM
 ledcAttachPin(ledPin, buzzer_ledChannel);        //GPIO Pin
 ledcWriteTone(buzzer_ledChannel, 500);           //Freq
 beep_beep();
  
 lightMeter.begin();
  
 pixels.setBrightness(150); // Set BRIGHTNESS to about 1/5 (max = 255)
 pixels.begin();
 
 tft.init();
 tft.setRotation(3);
 tft.fillScreen(TFT_BLACK);
 tft.setTextColor(TFT_GREEN, TFT_BLACK);

 TJpgDec.setJpgScale(2);  //The jpeg image can be scaled by a factor of 1, 2, 4, or 8
 TJpgDec.setSwapBytes(true);  //The byte order can be swapped (set true for TFT_eSPI)
 TJpgDec.setCallback(tft_output);  //The decoder must be given the exact name of the rendering function above

 hdc1080.begin(0x40);

 pinMode(TFT_BACKLIGHT, OUTPUT);

 //PWM Stuff
 ledcSetup(ledChannel, freq, resolution);    // configure LED PWM functionalitites
 ledcAttachPin(TFT_BACKLIGHT , ledChannel);  // attach the channel to the GPIO to be controlled 

 Brighten_Display();
  
 Serial.println("Adafruit VL53L0X test");
 if (!lox.begin()) 
 {
  Serial.println(F("Failed to boot VL53L0X"));
  while(1);
 }

 if (! sgp.begin())
 {
  Serial.println("Sensor not found :(");
  while (1);
 }
 Serial.print("Found SGP30 serial #");
 Serial.print(sgp.serialnumber[0], HEX);
 Serial.print(sgp.serialnumber[1], HEX);
 Serial.println(sgp.serialnumber[2], HEX);

 // Connect to Wi-Fi
 Serial.print("Connecting to ");
 tft.println("Connecting to Wifi");
 Serial.println(ssid);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) 
 {
  delay(500);
  Serial.print(".");
  tft.print(".");
 }

 tft.println("");
 tft.println("WiFi connected.");
 tft.println("IP address: ");
 tft.println(WiFi.localIP());
 delay(3000);
 
 //Initialize a NTPClient to get time
 timeClient.begin();
 timeClient.setTimeOffset(-10800);  //My timezone

 targetTime = millis() + 1000; 
 delay(500);
 tft.fillScreen(TFT_BLACK);
 timeClient.update();
} 

int counter = 0; int temp; int red; int green;
void loop() 
{
  VL53L0X_RangingMeasurementData_t measure;
  static int iOldDistance, iDistance;
  int iChanged = 0;
  if (! sgp.IAQmeasure()) 
  {
   Serial.println("Measurement failed");
   return;
  }

  if (! sgp.IAQmeasureRaw()) 
  {
   Serial.println("Raw Measurement failed");
   return;
  }
  
  pixels.clear();
  Serial.println(sgp.eCO2);
  temp = sgp.eCO2;
  temp = constrain(temp, 400, 1500);
  Serial.println(temp);
  red = map(temp, 400, 1500, 0, 250);
  green = 250 - red; // since the range is 0 to 250
  Serial.print("Mapped: ");
  Serial.println(red);

  pixels.setPixelColor(0, pixels.Color(red, green, 0));
  pixels.show(); 

  //LightMeter
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx"); 
  
  measure_CO2();
   
  unsigned long currentMillis = millis(); 
  ledcWrite(ledChannel, 20); //Display brightness dim
   
  if (currentMillis - previousMillis >= interval) 
  {
   previousMillis = currentMillis; 
   timeClient.update();
   Serial.println("Time Updated");
   Serial.print("eCO2 "); Serial.println(sgp.eCO2); 
   Serial.println("CO2 Level");
  }
       
  Serial.print("Reading a Range Distance");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
 
  if (measure.RangeStatus != 4) 
  {
   Serial.print("Distance (mm): "); Serial.println(iDistance);
  } 
  else 
  {
   Serial.println(" out of range ");
  } 
 
  Serial.print("Distance (mm): "); Serial.println(iDistance);
  unsigned long epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  

  int currentHour = timeClient.getHours();
  Serial.print("Hour: ");
  Serial.println(currentHour);  

  int currentMinute = timeClient.getMinutes();
  Serial.print("Minutes: ");
  Serial.println(currentMinute); 
   
  int currentSecond = timeClient.getSeconds();
  Serial.print("Seconds: ");
  Serial.println(currentSecond);  
  
  iDistance = (int)measure.RangeMilliMeter;

  if ((iDistance > 25) && (iDistance < 400))// Hand over sensor
  { 
   getData();  //Could be too slow
   formattedDate = timeClient.getFormattedTime();
   int splitT = formattedDate.indexOf("T");
   dayStamp = formattedDate.substring(0, splitT); 
   tft.fillScreen(TFT_BLACK);
   tft.drawLine( 0, 0, 240, 0, TFT_WHITE);   //Line top across
   tft.drawLine( 0, 0, 0, 135,TFT_WHITE);    //Line top down
   tft.drawLine( 0, 134, 240, 134, TFT_WHITE);  //Line bottom across
   tft.drawLine( 239, 0, 239, 134, TFT_WHITE);  //Line at end, down
   tft.drawLine( 0, 63, 148, 63, TFT_WHITE);    //Line at middle across
   tft.drawLine( 148, 0, 148, 148,TFT_WHITE);  //Aqua=0x04FF, Yellow = 0xFFE0,80, 0, 80, 80
   tft.setCursor(7, 20);  //over, down
   tft.setFreeFont(&Orbitron_Medium_20);
   tft.setTextColor(TFT_WHITE,TFT_BLACK);
   tft.setTextSize(1);
   tft.print("Low Temp: ");
   tft.setFreeFont(&Orbitron_Medium_20);
   tft.setCursor(35, 45);
   tft.setTextColor(TFT_aqua,TFT_BLACK);
   tft.print(tempmin); 
   tft.print(" `");  
   tft.print("C");

   tft.setCursor(7, 90);  //over, down
   tft.setTextColor(TFT_WHITE,TFT_BLACK);
   tft.setTextSize(1);
   tft.print("High Temp: ");
   tft.setFreeFont(&Orbitron_Medium_20);
   tft.setCursor(35, 120);
   tft.setTextColor(TFT_darkred,TFT_BLACK);
   tft.print(tempmax);
   tft.print(" `");  
   tft.print("C");

   if(id == 501)
   {
    TJpgDec.drawJpg(160, 25, rain, sizeof(rain));   //// Draw the image, over and down, 100, 20
   }

   if(id == 800)
   {
    TJpgDec.drawJpg(160, 25, sun, sizeof(sun));   //// Draw the image, over and down
   }

   if(id == 801 || id == 802 || id == 803 || id == 804)
   {
    TJpgDec.drawJpg(155, 25, cloudy, sizeof(cloudy));   //// Draw the image, over and down
   }

   else
   { 
   }

   //233=storm, 499=showers, 501=rain, 599=snow, 699=fog, 802=cloud, 804-cloud
   
   //Draw weather Index number
   tft.setCursor(175, 18);  //over, down
   tft.setTextColor(TFT_WHITE,TFT_BLACK);
   tft.setTextSize(1);
   tft.print(id);
   
   Brighten_Display();
     
   delay(5000);
   Dim_Display();
   tft.fillScreen(TFT_BLACK);  
  } 
 
  else 
  {
   ledcWrite(ledChannel, 20); //Keep dim
  }

  byte xpos = 40; //10 over
  byte ypos = 45; //15 down
    
  if (omm != currentMinute) 
  { 
   tft.setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
   tft.drawString("88:88",xpos,ypos,7); // Overwrite the text to clear it
   tft.setTextColor(0xF9C0); // Orange
   omm = currentMinute;

   if (currentHour > 12) currentHour -= 12;
   else if (currentHour == 0) currentHour += 12;
     
   if (currentHour < 10)
   {
    xpos+= 32; 
   }
            
   xpos+= tft.drawNumber(currentHour,xpos,ypos,7); 
   xcolon=xpos;
   xpos+= tft.drawChar(':',xpos,ypos,7);
      
   if (currentMinute<10) xpos+= tft.drawChar('0',xpos,ypos,7);
   tft.drawNumber(currentMinute,xpos,ypos,7);
  }

  if (currentSecond%2) // Flash the colon
  { 
   tft.setTextColor(0x39C4, TFT_BLACK);
   xpos+= tft.drawChar(':',xcolon,45,7);  
   tft.setTextColor(0xFBE0, TFT_BLACK);
  }

  else 
  {
   tft.drawChar(':',xcolon,45,7);
   tft.setTextColor(0xF9C0, TFT_BLACK);
  }
} 

void getData()
{
 tft.fillRect(1,170,64,20,TFT_BLACK);
 tft.fillRect(1,210,64,20,TFT_BLACK);
 if ((WiFi.status() == WL_CONNECTED))
 { 
  HTTPClient http;
  http.begin(endpoint + key); //Specify the URL
  int httpCode = http.GET();  //Make the request
 
  if (httpCode > 0)
  { 
   payload = http.getString();
   Serial.println(payload);   
  }

  else 
  {
   Serial.println("Error on HTTP request");
  }
 
  http.end(); 
 }
  char inp[1000];
  payload.toCharArray(inp,1000);
  deserializeJson(doc,inp);
 
  id = doc["weather"][0]["id"];
  String description = doc["weather"][0]["description"];
    
  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  String tempmin2 = doc["main"]["temp_min"];
  String tempmax2 = doc["main"]["temp_max"];

  tmp=tmp2;
  hum=hum2;
  tempmin=tempmin2;
  tempmax=tempmax2;
  
  Serial.println(id); 
  Serial.println(description);  //Weather Description
  Serial.println("Temp: "+String(tmp));
  Serial.println("Humidity: "+hum);
  Serial.println("Temp Min: "+tempmin);
  Serial.println("Temp Max: "+tempmax); 
}
         
void measure_CO2()
{ 
 if (! sgp.IAQmeasure()) 
  {
   Serial.println("Measurement failed");
   return;
  }
  Serial.print("eCO2 ");
  Serial.print(sgp.eCO2);
  Serial.println(" ppm");
  tft.setTextColor(0xF9C0, TFT_BLACK);
  char temp_out[] = " ";
  float temperature_sensor = hdc1080.readTemperature();
  dtostrf(temperature_sensor,4,1,temp_out);
  tft.drawString(temp_out, 0, 0, 4);
  tft.drawString("'C", 55, 0, 4);

  char humidity_out[] = " ";
  float humidity_sensor = hdc1080.readHumidity();
  dtostrf(humidity_sensor,4,1,humidity_out);
  tft.drawString(humidity_out, 135, 0, 4);
  tft.drawString("%H", 195, 0, 4);

  tft.setTextColor(0xF9C0, TFT_BLACK);
  tft.drawString("C02: ", 0, 115, 4);
  char co2_buf[8]; dtostrf(sgp.eCO2, 4, 0, co2_buf);    //Super Fast Refresh!
  tft.drawString(co2_buf, 50, 115, 4);
  Serial.println("CO2 Measured !!!!!!!!");

  tft.setTextColor(0xF9C0, TFT_BLACK);
  tft.drawString("TVOC: ", 122, 115, 4);
  char tvoc_buf[8]; dtostrf(sgp.TVOC, 4, 0, tvoc_buf);    //Super Fast Refresh!
  tft.drawString(co2_buf, 185, 115, 4);
  Serial.println("TVOC Measured !!!!!!!!");

  if (! sgp.IAQmeasureRaw()) 
  {
   Serial.println("Raw Measurement failed");
   return;
  }
 
  counter++;
  if (counter == 30) 
  {
   counter = 0;
   uint16_t TVOC_base, eCO2_base;
   if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) 
   {
    Serial.println("Failed to get baseline readings");
    return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
}


void beep_beep() 
{
 //ledcWriteTone(buzzer_ledChannel, 500);                         
 //delay(50);  
 //ledcWriteTone(buzzer_ledChannel, 0);                       
 //delay(50); 
 //ledcWriteTone(buzzer_ledChannel, 500);                       
 //delay(50);  
 ledcWriteTone(buzzer_ledChannel, 0);                                               
}

     
