//Great Example of weather data firmatting: https://github.com/xxlukas42/SensorlessWeatherStation/blob/master/sensorless_weather_station.ino
//Create C arrays from jpeg images using this online tool:
//    http://tomeko.net/online_tools/file_to_hex.php?lang=en

//Issues:
//24H to 12H always adds a shifted 1/2 space in front over the gray 88:88
//after sleeping, awake and then sleep, display only shows :, possibly becuase of if (omm != currentMinute) 


//In User_Setup.h remove
//#define TFT_BL   26            
//#define TFT_BACKLIGHT_ON HIGH 

#include <Wire.h>
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

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
ClosedCube_HDC1080 hdc1080;
Adafruit_SGP30 sgp;
Adafruit_NeoPixel strip(1, 5, NEO_GRB + NEO_KHZ800);

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
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

// Replace with your network credentials
const char *ssid     = "SSID";  
const char *password = "PASSWORD";     

//Open Weather Settings
String town="Hantsport";              //EDDIT
String Country="CA";                //EDDIT
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
const String key = "KEY"; /*Open Weather API Key*/
String formattedDate;
String dayStamp;

String payload=""; //whole json 
String tmp="" ; //temperature
String hum="" ; //humidity
String weather="" ; //weather
String description="" ; //description
String tempmin="" ; //temperature
String tempmax="" ; //temperature
int id;

int weatherID = 0;    


StaticJsonDocument<1000> doc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

//uint8_t ss, mm, hh;

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
  strip.begin();           
  strip.show();           
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  TJpgDec.setJpgScale(2);  //The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setSwapBytes(true);  //The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setCallback(tft_output);  //The decoder must be given the exact name of the rendering function above

  Wire.begin(13,14);
  delay(150);

  hdc1080.begin(0x40);

  pinMode(TFT_BACKLIGHT, OUTPUT);

  //PWM Stuff
  ledcSetup(ledChannel, freq, resolution);    // configure LED PWM functionalitites
  ledcAttachPin(TFT_BACKLIGHT , ledChannel);  // attach the channel to the GPIO to be controlled 

  Brighten_Display();
  
  Serial.begin(115200);

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
 
  //tft.fillScreen(TFT_BLACK);

// Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(-10800);  //My timezone

  targetTime = millis() + 1000; 
  //getData();
  delay(500);
  tft.fillScreen(TFT_BLACK);
  timeClient.update();
} 

int counter = 0;
void loop() 
{
  //colorWipe(strip.Color(255,   0,   0), 250); // Red
  //colorWipe(strip.Color(  0, 255,   0), 250); // Green
  
  VL53L0X_RangingMeasurementData_t measure;
  static int iOldDistance, iDistance;
  int iChanged = 0;

  //unsigned long currentMillis = millis();

  while (1) 
  { 

   tft.fillRect(64,112,75,28,TFT_BLACK);  //Erases old CO2 value
   tft.fillRect(0,0,240,20,TFT_BLACK);  //Erases old temp/humidity values
   //tft.fillRect(x, y, length of rectangle, width of rectangle, color 

   measure_CO2();
    
  
   unsigned long currentMillis = millis(); 
   ledcWrite(ledChannel, 20); //Display brightness dim
   
   
   if (currentMillis - previousMillis >= interval) 
   {
    previousMillis = currentMillis; 
    timeClient.update();
    Serial.println("Time Updated!!!!!!!!!!!!!!!!!!!!!");
    
    Serial.print("eCO2 "); Serial.println(sgp.eCO2); 
    Serial.println("CO2 Level!!!!!!!!!!!!!!!!!!!!!");
   }
       
   delay(250);
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
    
    //tft.fillRect(70,42,5,20,TFT_GREY);     //Draws a grey line in the middle, over,height,width,length
    //tft.drawLine( 0, 0, 150, 0,TFT_WHITE);  //X0,Y0,X1,Y1,color
    tft.drawLine( 0, 0, 240, 0, TFT_WHITE);   //Line top across
    tft.drawLine( 0, 0, 0, 135,TFT_WHITE);    //Line top down
    tft.drawLine( 0, 134, 240, 134, TFT_WHITE);  //Line bottom across
    tft.drawLine( 239, 0, 239, 134, TFT_WHITE);  //Line at end, down
    tft.drawLine( 0, 63, 148, 63, TFT_WHITE);    //Line at middle across
    

    
    tft.drawLine( 148, 0, 148, 148,TFT_WHITE);  //Aqua=0x04FF, Yellow = 0xFFE0,80, 0, 80, 80
    
    tft.setCursor(3, 20);  //over, down
    tft.setFreeFont(&Orbitron_Medium_20);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.setTextSize(1);
    tft.print("Low Temp: ");
    tft.setFreeFont(&Orbitron_Medium_20);
    tft.setCursor(3, 45);
    tft.setTextColor(TFT_aqua,TFT_BLACK);
    tft.print(tempmin); 
    //tft.print(" Deg C");
    tft.print(" `");  
    tft.print("C");

    tft.setCursor(3, 100);  //over, down
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.setTextSize(1);
    tft.print("High Temp: ");
    tft.setFreeFont(&Orbitron_Medium_20);
    tft.setCursor(3, 130);
    tft.setTextColor(TFT_darkred,TFT_BLACK);
    tft.print(tempmax);
    //tft.print(" Deg C");
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
     TJpgDec.drawJpg(160, 25, cloudy, sizeof(cloudy));   //// Draw the image, over and down
    }


    else
   {
    
   }

    //233=storm, 499=showers, 501=rain, 599=snow, 699=fog, 802=cloud, 804-cloud

    //Draw weather Index number
    tft.setCursor(160, 18);  //over, down
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

   
   //tft.setCursor(0, 15);  //over, down
   //tft.setFreeFont(&Orbitron_Medium_20);
   //tft.setTextColor(0xF9C0); // Orange
   //tft.setTextSize(1);
   //tft.print("Mike");
   //Serial.println("Mike");
   
  

   byte xpos = 40; //10 over
    byte ypos = 45; //15 down
    
     if (omm != currentMinute) 
     { // Only redraw every minute to minimise flicker
      tft.setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
      // Font 7 is to show a pseudo 7 segment display.
      // Font 7 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 0 : .
      tft.drawString("88:88",xpos,ypos,7); // Overwrite the text to clear it
      tft.setTextColor(0xF9C0); // Orange
      omm = currentMinute;


       
    
      if (currentHour > 12) currentHour -= 12;
      else if (currentHour == 0) currentHour += 12;
     
      
      if (currentHour < 10)
      {
       xpos+= 32; 
       //xpos+= tft.drawChar('-',xpos,ypos,7);  //10,15,7
       //xpos+= tft.drawNumber(currentHour,xpos,ypos,7); //hour shifted
      }
      //If I place '0' O see a zero in front and it's spaced properly
      //If I place ' ' then the digits are shifted wrong

//     */
//       else if (currentHour > 12)
//       {
//        currentHour = currentHour- 12;
        //xpos+= tft.drawNumber(currentHour,xpos,ypos,7); 
//        xpos+= tft.drawChar('-',xpos,ypos,7);
        //xpos+= tft.drawNumber(currentHour,xpos,ypos,7); //hour shifted
//       }

      
      
      xpos+= tft.drawNumber(currentHour,xpos,ypos,7);
      
      xcolon=xpos;
      xpos+= tft.drawChar(':',xpos,ypos,7);
      
      if (currentMinute<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      tft.drawNumber(currentMinute,xpos,ypos,7);
    }

     if (currentSecond%2) 
     { // Flash the colon
      tft.setTextColor(0x39C4, TFT_BLACK);
      xpos+= tft.drawChar(':',xcolon,45,7);  //15,7
      tft.setTextColor(0xFBE0, TFT_BLACK);
     }

      else 
    {
      tft.drawChar(':',xcolon,45,7);
      colour = random(0xFFFF);
      // Erase the old text with a rectangle, the disadvantage of this method is increased display flicker
      //tft.fillRect (0, 64, 160, 20, TFT_BLACK);
      
      //tft.fillRect (0, 0, 240, 135, TFT_BLACK);  //Full black
      //tft.fillRect (0,85,240,50, TFT_BLACK);   //lower bottom black
      //tft.drawChar('M',0,0,7);
    }

   
   
 
    //tft.fillRect(64,112,75,28,TFT_BLACK);  //Erases old CO2 value
    //tft.fillRect(0,0,240,20,TFT_BLACK);  //Erases old temp/humidity values
    //tft.fillRect(x, y, length of rectangle, width of rectangle, color
  } 
} 


void getData()
{
    tft.fillRect(1,170,64,20,TFT_BLACK);
    tft.fillRect(1,210,64,20,TFT_BLACK);
   if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
         payload = http.getString();
       // Serial.println(httpCode);
        Serial.println(payload);
        
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
 char inp[1000];
 payload.toCharArray(inp,1000);
 deserializeJson(doc,inp);


  //int id = doc["weather"][0]["id"];
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
  //Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 ");
  Serial.print(sgp.eCO2);
  Serial.println(" ppm");

  tft.setCursor(0, 130);  //over, down
  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setTextColor(0xF9C0); // Orange
  tft.setTextSize(1);
  tft.print("C02: ");
  tft.print(sgp.eCO2);
  Serial.println("CO2 Measured !!!!!!!!");

  tft.setCursor(0, 20);  //over, down
  tft.print(hdc1080.readTemperature());
  tft.print("`");  
  tft.print("C");

  tft.setCursor(135, 20);  //over, down
  tft.print(hdc1080.readHumidity());
  tft.print("%");
  
  Serial.println("Humidity and Temp Measured !!!!!!!!");
  

  if (! sgp.IAQmeasureRaw()) 
  {
   Serial.println("Raw Measurement failed");
   return;
  }
  //Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  //Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
  delay(1000);

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


void colorWipe(uint32_t color, int wait) 
{
  for(int i=0; i<strip.numPixels(); i++) 
  { 
   strip.setPixelColor(i, color);         
   strip.show();                         
   delay(wait);                          
  }
}
     
