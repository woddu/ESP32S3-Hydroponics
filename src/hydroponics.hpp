#pragma once
#include <Arduino.h>
#include <Time.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "tft.hpp"
#include "tds.hpp"
#include "phsensor.hpp"
#include "pins.hpp"
#include "datetime.hpp"


#define LOG_DRAINING_STOP       "Log12"
#define DHTTYPE    DHT22

BLECharacteristic *pNotifyLogs = NULL;

TaskHandle_t xTFTLeftPanelHandle = NULL;
TaskHandle_t xTFTChildPanelHandle = NULL;
SemaphoreHandle_t xTFTPrintMutex;
SemaphoreHandle_t xADSMutex;

Adafruit_ADS1015 ads;

// Adafruit_ADS1015 LDRs;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

VL53L0X_RangingMeasurementData_t measure;

Preferences pref;

Servo servo;

DHT_Unified dht(DHT22_PIN, DHTTYPE);

int16_t ads2, ads3;
float volt2, volt3;

static bool startAttempt;
static bool pauseAttempt;
static bool drainAttempt;
static bool hasWater;
static bool prepared;
static bool started;
static bool paused;
static bool draining;
static bool reading;

bool waterPumpState, airPumpState, drainPumpState, servoState, growLightState, humidifierState;

float dhtTemp, dhtHumid;

static bool phRead;

int lightTreshold;

int waterLevel;

static int prevLightHour = 18;
static int lightHour = 18;

static int prevDarkHour = 6;
static int darkHour = 6;

int lightDarkCounter;

inline bool countingLight = true;

inline bool Starting() {
  return startAttempt && !prepared && !started;
}

inline bool Pausing() {
  return pauseAttempt && started;
}

inline bool Resuming() {
  return startAttempt && started;
}

inline bool Prepared() {
  return !startAttempt && prepared && !started;
}

inline bool Started() {
  return started && prepared && !paused;
}

inline bool Paused() {
  return started && paused;
}

inline bool Draining() {
  return draining;
}

inline bool NotRunning() {
  return !started && !paused && !prepared;
}

inline unsigned long LightHour() {
  return (unsigned long)lightHour * HOUR;
}

inline unsigned long DarkHour() {
  return (unsigned long)darkHour * HOUR;
}


void TFTPrint(const int &x, const int &y, const String &print, const uint16_t &color = onSurfaceVariant, const int &size = 1){
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.getTextBounds(print, 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
  if ((print.indexOf('g') != -1) || (print.indexOf('y') != -1)){
    tft.setCursor(x, y + hBuf - (size == 1 ? 4 : 12));
  } else if((print.indexOf('q') != -1) || (print.indexOf('Q') != -1) || (print.indexOf('p') != -1)){
    tft.setCursor(x, y + hBuf - (size == 1 ? 3 : 9));
  } else {
    tft.setCursor(x, y + hBuf);
  }
  tft.print(print);
}

void RectWithCenteredText(
  const bool &fill, const int16_t &rectX, const int16_t &rectY, const int16_t &rectWidth, const int16_t &rectHeight, const uint16_t &rectColor, 
  const String &print, const uint16_t &color = onPrimaryContainer, const int &size = 1, const int16_t &margin = 0
){
  tft.setTextSize(size);

  tft.getTextBounds(print, 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
  
  if (wBuf > rectWidth) {
      rectWidth = wBuf + margin;
  }
  if (hBuf > rectHeight) {
      rectHeight = hBuf + margin;
  }
  
  if (fill){
    tft.fillRect(rectX, rectY, rectWidth, rectHeight, rectColor);
  } else {
    tft.drawRect(rectX, rectY, rectWidth, rectHeight, rectColor);
  }

  int16_t textX = rectX + ((rectWidth - wBuf) / 2);
  int16_t textY = rectY + ((rectHeight - hBuf) / 2);

  // Serial.printf("x: %d, y: %d, wBuf: %d, hBuf: %d \n", textX, textY, wBuf, hBuf);

  TFTPrint(textX - 2, textY - 2, print, color, size);
}

bool ScreenChanged() {
  if(GUI.screenChanged){
    GUI.screenChanged = !GUI.screenChanged;
    return !GUI.screenChanged;
  } else {
    return GUI.screenChanged;
  }
}

void TFTLeftPanel(void *param){
  uint32_t ulNotificationValue;

  while(1){
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY) == pdTRUE) {
      if(xSemaphoreTake(xTFTPrintMutex, portMAX_DELAY) == pdTRUE){
        Serial0.println("Notification Received!");
        
        tft.fillRect(0,60, 80, (180), surfaceVariant);
        switch (GUI.leftPanelSelectionY){
          case 0:
          RectWithCenteredText(true, 0, 60, 80, 37, secondary, "Actions", 0xFFFF);
          RectWithCenteredText(true, 0, 97, 80, 37, surfaceVariant, "Light", onSurfaceVariant);
          RectWithCenteredText(true, 0, (134), 80, 37, surfaceVariant, "Sensor", onSurfaceVariant);
          RectWithCenteredText(true, 0, (171), 80, 37, surfaceVariant, "Pump..", onSurfaceVariant);
          break;
        case 1:
          RectWithCenteredText(true, 0, 60, 80, 37, surfaceVariant, "Actions", onSurfaceVariant);
          RectWithCenteredText(true, 0, 97, 80, 37, secondary, "Light", 0xFFFF);
          RectWithCenteredText(true, 0, (134), 80, 37, surfaceVariant, "Sensor", onSurfaceVariant);
          RectWithCenteredText(true, 0, (171), 80, 37, surfaceVariant, "Pump..", onSurfaceVariant);
          break;
        case 2:
          RectWithCenteredText(true, 0, 60, 80, 37, surfaceVariant, "Actions", onSurfaceVariant);
          RectWithCenteredText(true, 0, 97, 80, 37, surfaceVariant, "Light", onSurfaceVariant);
          RectWithCenteredText(true, 0, (134), 80, 37, secondary, "Sensor", 0xFFFF);
          RectWithCenteredText(true, 0, (171), 80, 37, surfaceVariant, "Pump..", onSurfaceVariant);
          break;
        case 3:
          RectWithCenteredText(true, 0, 60, 80, 37, surfaceVariant, "Actions", onSurfaceVariant);
          RectWithCenteredText(true, 0, 97, 80, 37, surfaceVariant, "Light", onSurfaceVariant);
          RectWithCenteredText(true, 0, (134), 80, 37, surfaceVariant, "Sensor", onSurfaceVariant);
          RectWithCenteredText(true, 0, (171), 80, 37, secondary, "Pump..", 0xFFFF);
          break;
        default:
          break;
        }
        xSemaphoreGive(xTFTPrintMutex);
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
inline int ChildPanelX(const int &x){ return x + 80; }
inline int ChildPanelY(const int &y){ return y + 60; }
void TFTChildPanel(void *param){
  uint32_t ulNotificationValue;
  
  const int childPanelWidth = 240, childPanelHeight = 180;

  int randNum;

  String sampleStr;
  sampleStr.reserve(50);
  
  TFTPrint(ChildPanelX(12), ChildPanelY(94), "Date:", onSurface);
  TFTPrint(ChildPanelX(12), ChildPanelY(128), "Water Level:", onSurface);

  while(1){
    randNum = random(25, 35);

    if(xSemaphoreTake(xTFTPrintMutex, portMAX_DELAY) == pdTRUE){
      if (ScreenChanged()){
        tft.fillRect(80,60, childPanelWidth, childPanelHeight, surface);
        if (AtMainScreen()){
          TFTPrint(ChildPanelX(12), ChildPanelY(94), "Date:", onSurface);
          TFTPrint(ChildPanelX(12), ChildPanelY(128), "Water Level:", onSurface);
        } else if (AtActionScreen()){
          
        } else if(AtLightScreen()){
          tft.setFont(&FreeSans18pt7b);
          TFTPrint(ChildPanelX((childPanelWidth / 4) - 40), ChildPanelY(6), "Light", onSurface);
          TFTPrint(ChildPanelX((childPanelWidth / 4 * 3) - 40), ChildPanelY(10), "Dark", onSurface);
          TFTPrint(ChildPanelX((childPanelWidth / 4) - 40), ChildPanelY(childPanelHeight / 2) + 48 , "Hour", onSurface);
          TFTPrint(ChildPanelX((childPanelWidth / 4 * 3) - 40), ChildPanelY(childPanelHeight / 2) + 48 , "Hour", onSurface);
          tft.setFont(&FreeSans12pt7b);

          tft.fillTriangle(ChildPanelX(childPanelWidth / 2), ChildPanelY(childPanelHeight / 2) - 16, ChildPanelX(childPanelWidth / 2) - 8, ChildPanelY(childPanelHeight / 2) - 4, ChildPanelX(childPanelWidth / 2) + 8, ChildPanelY(childPanelHeight / 2) - 4, primary);
          tft.fillTriangle(ChildPanelX(childPanelWidth / 2), ChildPanelY(childPanelHeight / 2) + 16, ChildPanelX(childPanelWidth / 2) - 8, ChildPanelY(childPanelHeight / 2) + 4, ChildPanelX(childPanelWidth / 2) + 8, ChildPanelY(childPanelHeight / 2) + 4, primary);

        } else if (AtSensorScreen()){
          tft.fillRect(0,60, (320), (180), surface);
          
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 )) - 24, "pH: ", onSurface);Serial0.printf("y: %d", tft.getCursorY());
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 * 2)) - 24, "Water: ", onSurface);Serial0.printf("y: %d", tft.getCursorY());
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 * 3)) - 24, "Temp: ", onSurface);Serial0.printf("y: %d", tft.getCursorY());
            
          TFTPrint((320 / 12 * 8) - 70, ChildPanelY((childPanelHeight / 4 )) - 24, "TDS: ", onSurface);Serial0.printf("y: %d", tft.getCursorY());
          TFTPrint((320 / 12 * 8) - 40, ChildPanelY((childPanelHeight / 4 * 2)) - 24, "Humid: ", onSurface);Serial0.printf("y: %d\n", tft.getCursorY());

          tft.fillRect(0, 203, 320, 37, surfaceVariant);
          TFTPrint((320 - 73) / 2, 212, "Sensors");
          tft.fillTriangle(6, (203 + (37 / 2)), 24, 209, 24, 234, secondary);
        } else if (AtPumpScreen()){
          tft.fillRect(0,60, (320), (180), surface);
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 )) - 24, "Water: ", onSurface);Serial0.printf("y: %d", tft.getCursorX());
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 * 2)) - 24, "Drain: ", onSurface);Serial0.printf("y: %d", tft.getCursorX());
          TFTPrint((320 / 12 * 2) - 40, ChildPanelY((childPanelHeight / 4 * 3)) - 24, "Servo: ", onSurface);Serial0.printf("y: %d", tft.getCursorX());

          TFTPrint((320 / 12 * 8) - 40, ChildPanelY((childPanelHeight / 4 )) - 24, "Air: ", onSurface);Serial0.printf("y: %d", tft.getCursorX());
          TFTPrint((320 / 12 * 8) - 40, ChildPanelY((childPanelHeight / 4 * 2)) - 24, "LED: ", onSurface);Serial0.printf("y: %d", tft.getCursorX());
          TFTPrint((320 / 12 * 8) - 40, ChildPanelY((childPanelHeight / 4 * 3)) - 24, "Humid: ", onSurface);Serial0.printf("y: %d\n", tft.getCursorX());

          tft.fillRect(0, 203, 320, 37, surfaceVariant);
          TFTPrint((320 - 147) / 2, 210, "Pumps & etc...");
          tft.fillTriangle(6, (203 + (37 / 2)), 24, 209, 24, 234, secondary);
        }
      }

      if (AtMainScreen()){
        vTaskDelay(10/ portTICK_PERIOD_MS);
        tft.setFont(&FreeSans18pt7b);
        if (Starting()){
          if(phRead == 1){
            tft.getTextBounds("Reading", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "Reading", onSurface);  
          } else if(phRead == 2){
            tft.getTextBounds("Low pH", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "Low pH", onSurface);  
          } else if(phRead == 3){
            tft.getTextBounds("High pH", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "High pH", onSurface);  
          } else if(phRead == 0){
            tft.getTextBounds("Starting", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "Starting", onSurface);  
          }
        } else if (Started()){
          tft.getTextBounds("Running", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
          // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
          TFTPrint(ChildPanelX(58), ChildPanelY(14), "Running", onSurface);  
        } else if (Paused()){
          tft.getTextBounds("Paused", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
          // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
          TFTPrint(ChildPanelX(64), ChildPanelY(14), "Paused", onSurface);  
        } else if (Draining()){
          tft.getTextBounds("Draining", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
          // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
          TFTPrint(ChildPanelX(64), ChildPanelY(14), "Draining", onSurface);  
        } else if(NotRunning()){
          if(phRead == 2){
            tft.getTextBounds("Low pH", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "Low pH", onSurface);  
          } else if(phRead == 3){
            tft.getTextBounds("High pH", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(61), ChildPanelY(14), "High pH", onSurface);  
          } else if(phRead == 0 || phRead == 1){
            tft.getTextBounds("Not Running", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
            // Serial0.printf("x: %d\n", (childPanelWidth - wBuf) / 2);
            TFTPrint(ChildPanelX(22), ChildPanelY(14), "Not Running", onSurface);  
          }
        }
        tft.setFont(&FreeSans12pt7b);
        time_t runTime = runtimeTimeStamp;
        struct tm *timeInfo = localtime(&runTime); // Convert to time structure

        char timeString[9];
        sprintf(timeString, "%02d:%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
        tft.getTextBounds(timeString, 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        RectWithCenteredText(true, ChildPanelX((childPanelWidth - wBuf) / 2), ChildPanelY((childPanelHeight - hBuf) / 10 * 4), 2, 2, surface, timeString, onSurface, 1, 5);

        time_t rawTime = time(nullptr); // Get current time in seconds
        struct tm *timeInfo1 = localtime(&rawTime); // Convert to time structure

        char dateString[9]; // MM/dd/YY format
        sprintf(dateString, "%02d/%02d/%02d", timeInfo1->tm_mon + 1, timeInfo1->tm_mday, timeInfo1->tm_year % 100);
        RectWithCenteredText(true, ChildPanelX(76), ChildPanelY((childPanelHeight - hBuf) / 10 * 6), 2, 2, surface, dateString, onSurface);
        
        tft.fillRect(ChildPanelX(152), ChildPanelY((childPanelHeight - hBuf) / 10 * 8), 100, 70, surface);
        if(waterLevel >= 0 && waterLevel <= 33){
          RectWithCenteredText(true, ChildPanelX(156), ChildPanelY((childPanelHeight - hBuf) / 10 * 8), 2, 2, surface, "Low", onSurface, 1, 4);
        } else if(waterLevel >= 34 && waterLevel < 66){
          RectWithCenteredText(true, ChildPanelX(156), ChildPanelY((childPanelHeight - hBuf) / 10 * 8), 2, 2, surface, "Medium", onSurface, 1, 4);
        } else if(waterLevel >= 67){
          RectWithCenteredText(true, ChildPanelX(156), ChildPanelY((childPanelHeight - hBuf) / 10 * 8), 2, 2, surface, "High", onSurface, 1, 4);
        }
      
        
      } else if (AtActionScreen()){
        tft.setFont(&FreeSans18pt7b);
        
        if(Started()){
            TFTPrint(164, ChildPanelY(26), "Start", surfaceVariant);
            TFTPrint(154, ChildPanelY(78), "Pause", primary);
          } else if (Paused() || NotRunning()) {
            TFTPrint(164, ChildPanelY(26), "Start", primary);
            TFTPrint(154, ChildPanelY(78), "Pause", surfaceVariant);
          }
        TFTPrint(160, ChildPanelY(130), "Drain", primary);
        if (GUI.actionSelectionY == 0){
          
          if(Started()){
            TFTPrint(164, ChildPanelY(26), "Start", surfaceVariant);
            TFTPrint(154, ChildPanelY(78), "Pause", primary);
            tft.drawRect(160, ChildPanelY(22), 79 + 4, 36, surface);
            tft.drawRect(150, ChildPanelY(74), 100 + 4, 36, primary);
            tft.drawRect(163, ChildPanelY(126), 73 + 4, 36, surface);
          } else if (Paused() || NotRunning()) {
            TFTPrint(164, ChildPanelY(26), "Start", primary);
            TFTPrint(154, ChildPanelY(78), "Pause", surfaceVariant);
            tft.drawRect(160, ChildPanelY(22), 79 + 4, 36, primary);
            tft.drawRect(150, ChildPanelY(74), 100 + 4, 36, surface);
            tft.drawRect(163, ChildPanelY(126), 73 + 4, 36, surface);
            
          }
        } else if (GUI.actionSelectionY == 1){
          tft.drawRect(160, ChildPanelY(22), 79 + 4, 36, surface);
          tft.drawRect(150, ChildPanelY(74), 100 + 4, 36, surface);
          tft.drawRect(157, ChildPanelY(126), 77 + 9, 36, primary);
          
        }

        
        
        
        tft.setFont(&FreeSans12pt7b);
        // RectWithCenteredText(true, )

      } else if (AtLightScreen()){
        
        if(GUI.lightSelectionX == 0){
          tft.fillRect((ChildPanelX((childPanelWidth / 4) - 40)), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, primaryContainer);
          TFTPrint((lightHour >= 10 ? 112 : 128), 133, std::to_string(lightHour).c_str(), onPrimaryContainer, 2);
          
          tft.fillRect(ChildPanelX((childPanelWidth / 4 * 3) - 40), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, surface);
          tft.drawRect(ChildPanelX((childPanelWidth / 4 * 3) - 40), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, primaryContainer);
          TFTPrint((darkHour >= 10 ? 232 : 248), 133, std::to_string(darkHour).c_str(), primaryContainer, 2);
        } else if (GUI.lightSelectionX == 1) {
          tft.fillRect(ChildPanelX((childPanelWidth / 4) - 40), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, surface);
          tft.drawRect(ChildPanelX((childPanelWidth / 4) - 40), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, primaryContainer);
          TFTPrint((lightHour >= 10 ? 112 : 128), 133, std::to_string(lightHour).c_str(), primaryContainer, 2);
          
          tft.fillRect(ChildPanelX((childPanelWidth / 4 * 3) - 40), ChildPanelY((childPanelHeight / 2) - 40), 80, 80, primaryContainer);
          TFTPrint((darkHour >= 10 ? 232 : 248), 133, std::to_string(darkHour).c_str(), onPrimaryContainer, 2);
        }
        
        
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY) == pdTRUE) {
          
        }
      } else if (AtSensorScreen()){
        tft.setTextColor(onSurface, surface);
        tft.fillRect(53, 81, 50, 22, surface);
        tft.getTextBounds(String(pH_Val), 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        tft.setCursor(54, 100);
        tft.print(pH_Val);
        // tft.setCursor(87, 126);
        tft.fillRect(86, 126, 60, 22, surface);
        tft.setCursor(87, 144);
        tft.getTextBounds(String(waterLevel), 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        tft.print(waterLevel);tft.print(" %");
        // tft.setCursor(84, 171);
        tft.fillRect(83, 171, 60, 22, surface);
        tft.setCursor(84, 190);
        tft.getTextBounds(String(dhtTemp), 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        tft.print(dhtTemp);tft.print(char(248));tft.println("C");
        // tft.setCursor(228, 81);
        tft.setCursor(198, 100);
        tft.getTextBounds(String(tdsValue), 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        tft.print(tdsValue);
        tft.fillRect(tft.getCursorX(), 81, 60, 22, surface);
        tft.print(" ppm");
        // tft.setCursor(232, 126);
        tft.fillRect(231, 126, 60, 22, surface);
        tft.setCursor(232, 126);
        tft.getTextBounds(String(dhtHumid), 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
        tft.print(dhtHumid);tft.print(" %");
        // tft.setCursor(247, 171);
        tft.fillRect(246, 171, 60, 22, surface);
      } else if (AtPumpScreen()){
        tft.setTextColor(onSurface, surface);
        // tft.setCursor(54, 81);
        tft.fillRect(83, 81, 50, 22, surface);
        tft.setCursor(84, 100);
        tft.print((waterPumpState ? "ON" : "OFF"));
        // tft.setCursor(87, 126);
        tft.fillRect(86, 126, 50, 22, surface);
        tft.setCursor(87, 144);
        tft.print((drainPumpState ? "ON" : "OFF"));
        // tft.setCursor(84, 171);
        tft.fillRect(83, 171, 60, 22, surface);
        tft.setCursor(84, 190);
        tft.print((servoState ? "ON" : "OFF"));
        // tft.setCursor(228, 81);
        tft.fillRect(227, 81, 60, 22, surface);
        tft.setCursor(228, 100);
        tft.print((airPumpState ? "ON" : "OFF"));
        // tft.setCursor(232, 126);
        tft.fillRect(231, 126, 60, 22, surface);
        tft.setCursor(232, 144);
        tft.print((growLightState ? "ON" : "OFF"));
        // tft.setCursor(247, 171);
        tft.fillRect(246, 171, 60, 22, surface);
        tft.setCursor(247, 190);
        tft.print((humidifierState ? "ON" : "OFF"));
      }
      xSemaphoreGive(xTFTPrintMutex);
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    vTaskDelay(20/ portTICK_PERIOD_MS);
  }
}

void Joystick(void *param){

  int prevX, prevY;
  int currX, currY;
  unsigned long prevMil, printM;
  while(1){
    if(xSemaphoreTake(xADSMutex, portMAX_DELAY) == pdTRUE){
      ads2 = ads.readADC_SingleEnded(2);
      ads3 = ads.readADC_SingleEnded(3);
      volt2 = ads.computeVolts(ads2);
      volt3 = ads.computeVolts(ads3);
      xSemaphoreGive(xADSMutex);
    }
    prevX = currX;
    prevY = currY;
    currX = ads2;
    currY = ads3;
    
    if (millis() - printM >= 500){
      printM = millis();
      Serial0.printf("x: %d, y: %d\n", ads2, ads3);
    }

    if (millis() - prevMil >= 1000 && (AtSensorScreen() || AtPumpScreen())){
      prevMil = millis();
      xTaskNotifyGive(xTFTChildPanelHandle);
    }

    if(prevX >= 200 && currX < 200){  // Right
      if (AtMainScreen()){
        LeftPanelEnter();
        
      } else if (AtActionScreen()){
        // Actions
        if(GUI.actionSelectionY == 0){
          if(Started()){
            GUI.actionSelectionY == 0;
            pauseAttempt = true;
            LeftPanelBack();
          } else if (Paused() || NotRunning()) {
            // if there is water
            GUI.actionSelectionY == 0;
            startAttempt = true;
            LeftPanelBack();
          }
        } else {
          if(Draining()){
            Serial0.println("Stopping Drain");
            GUI.actionSelectionY == 0;
            draining = false;
            LeftPanelBack();
            pNotifyLogs->setValue(LOG_DRAINING_STOP);
            pNotifyLogs->notify();
          } else {
            Serial0.println("Starting Drain");
            GUI.actionSelectionY == 0;
            drainAttempt = true;
            LeftPanelBack();
          }
        }
        xTaskNotifyGive(xTFTChildPanelHandle);
      } else if (AtLightScreen()){
        GUI.lightSelectionX = 1;
        xTaskNotifyGive(xTFTChildPanelHandle);
      } else if (AtSensorScreen()){
  
      }
    } else if(prevX <= 900 && currX > 900){ // Left
      if (AtActionScreen()){
        xTaskNotifyGive(xTFTChildPanelHandle);
        LeftPanelBack();
      } else if (AtLightScreen()){
        if (GUI.lightSelectionX == 0) {
          xTaskNotifyGive(xTFTChildPanelHandle);
          LeftPanelBack();
        } else if(GUI.lightSelectionX == 1){
          GUI.lightSelectionX = 0;
          xTaskNotifyGive(xTFTChildPanelHandle);
        }
      } else if (AtSensorScreen()){
        LeftPanelBack();
        xTaskNotifyGive(xTFTLeftPanelHandle);
      } else if (AtPumpScreen()){
        LeftPanelBack();
        xTaskNotifyGive(xTFTLeftPanelHandle);
      }
    }

    if(prevY <= 900 && currY > 900){// Down
      if (AtMainScreen()){
        LeftPanelDown();
        xTaskNotifyGive(xTFTLeftPanelHandle);
      } else if (AtActionScreen()){
        GUI.actionSelectionY = 1;
        xTaskNotifyGive(xTFTChildPanelHandle);
      } else if (AtLightScreen()){
        if (GUI.lightSelectionX == 0) {
          lightHour = (lightHour <= 12 ? 12 : lightHour - 1);
          darkHour = 24 - lightHour;
          // pref.begin("iQuaRoots");
          // pref.putInt("LightHour", lightHour);
          // pref.putInt("DarkHour", darkHour);
          // pref.end();
          xTaskNotifyGive(xTFTChildPanelHandle);
        } else if(GUI.lightSelectionX == 1){
          darkHour = (darkHour <= 0 ? 0 : darkHour - 1);
          lightHour = 24 - darkHour;
          // pref.begin("iQuaRoots");
          // pref.putInt("LightHour", lightHour);
          // pref.putInt("DarkHour", darkHour);
          // pref.end();
          xTaskNotifyGive(xTFTChildPanelHandle);
        }
      } else if (AtSensorScreen()){
        
      } else if (AtPumpScreen()){
        
      }
    } else if(prevY >= 200 && currY < 200){// Up
      if (AtMainScreen()){
        LeftPanelUp();
        xTaskNotifyGive(xTFTLeftPanelHandle);
      } else if (AtActionScreen()){
        GUI.actionSelectionY = 0;
        xTaskNotifyGive(xTFTChildPanelHandle);
      } else if (AtLightScreen()){
        if (GUI.lightSelectionX == 0) {
          lightHour = (lightHour >= 24 ? 24 : lightHour + 1);
          darkHour = 24 - lightHour;
          // pref.begin("iQuaRoots");
          // pref.putInt("LightHour", lightHour);
          // pref.putInt("DarkHour", darkHour);
          // pref.end();
          xTaskNotifyGive(xTFTChildPanelHandle);
        } else if(GUI.lightSelectionX == 1){
          darkHour = (darkHour >= 12 ? 12 : darkHour + 1);
          lightHour = 24 - darkHour;
          // pref.begin("iQuaRoots");
          // pref.putInt("LightHour", lightHour);
          // pref.putInt("DarkHour", darkHour);
          // pref.end();
          xTaskNotifyGive(xTFTChildPanelHandle);
        }
      } else if (AtSensorScreen()){
        
      } else if (AtPumpScreen()){
        
      }
    }

    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}