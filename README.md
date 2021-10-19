# ESP32_Desktop_Clock
ESP32 based Desktop Clock

![Image1](https://user-images.githubusercontent.com/4991664/137914889-fa3bfda3-6b23-425e-a0d9-011137bf89a1.jpg)

![Image2](https://user-images.githubusercontent.com/4991664/137914879-f41ecba5-b2aa-40b7-b652-fe6137cbe4f0.jpg)

![Front_Components](https://user-images.githubusercontent.com/4991664/137002687-6ac9a98c-8455-4430-a600-dcdc6465f5c8.jpg)

![Back_Components](https://user-images.githubusercontent.com/4991664/137002693-1184a481-bafe-432f-8653-25b9ee9dacc8.jpg)


# Click below for a video of it doing its thing
[![Video of it doing its thing](https://img.youtube.com/vi/Y3FI6KhXrEo/0.jpg)](https://www.youtube.com/watch?v=Y3FI6KhXrEo)

This ESP32 internet of things desktop clock is a project created for my home. School has started and fall is coming so every morning my kids ask me what they should wear. This clock is normally very, very dim and shows the time, co2 value, room temperature and humidity in a dark orange color. When you move your hand a foot or so in front, the time fades away to a screen showing the days low temperature, high and what the weather conditions will be. Time is updated every few minutes over wifi from an NTP server, local weather data is updated from Openweather with various sensors on the front. 

Hardware on this board:
-ESP32 Pico D4 processor
-temperature/humidity sensor (HDC1080DMBT)
-laser range sensor (VL53L0CXV0DH)
-CO2/TVOC (SGP-30-2.5K)
-ambient light sensor (BH1750FVI)
-135x240 TFT LCD (ER-TFT1.14-1)
-3.3V LDO (NCP1117LPST33T3G)
-1.8V LDO (RT9193-18GB)
-Neopixel LED
-USB interface (CP2104N)
-USB-C Connector
-QwiiC i2c Connector
-Buzzer
