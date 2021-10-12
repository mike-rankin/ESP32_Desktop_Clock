# ESP32_Desktop_Clock
ESP32 based Desktop Clock

![Front_Components](https://user-images.githubusercontent.com/4991664/137002687-6ac9a98c-8455-4430-a600-dcdc6465f5c8.jpg)

![Back_Components](https://user-images.githubusercontent.com/4991664/137002693-1184a481-bafe-432f-8653-25b9ee9dacc8.jpg)

This ESP32 internet of things desktop clock is a project created for my home. School has started and fall is coming so every morning my kids ask me what they should wear. This clock is normally very, very dim and shows the time, co2 value, room temperature and humidity in a dark orange color. When you move your hand a foot or so in front, the time fades away to a screen showing the days low temperature, high and what the weather conditions will be. Time is updated every few minutes over wifi from an NTP server, local weather data is updated from Openweather with various sensors on the front. 

Hardware on this board:
-temperature/humidity sensor (HDC1080DMBT)
-Laser range sensor (VL53L0CXV0DH)
-accelerometer (LIS3DHTR)
-LiPo protection (DW01A)
-80x160 TFT LCD (ER-TFT0.96-1)
-3.3V LDO (HT7833)
-LiPo charger (SL4054ST25P)
-USB interface (CP2104N)
