#pragma once

#include <SPI.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include "pins.hpp"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

struct GUI_Attr {
  int leftPanelSelectionX;
  int leftPanelSelectionY;
  int actionSelectionY;
  int lightSelectionX;
  bool screenChanged;
} GUI;

uint16_t primary = tft.color565(62, 103, 0);
uint16_t secondary = tft.color565(75, 103, 42);
uint16_t tertiary = tft.color565(0, 105, 78);
uint16_t surface = tft.color565(247, 251, 234);
uint16_t onSurface = tft.color565(25, 29, 19);
uint16_t surfaceVariant = tft.color565(222, 230, 205);
uint16_t onSurfaceVariant = tft.color565(66, 73, 55);
uint16_t primaryContainer = tft.color565(79, 130, 0);
uint16_t onPrimaryContainer = tft.color565(249, 255, 234);
  
int16_t xBuf, yBuf; 
uint16_t wBuf, hBuf;

bool AtMainScreen() { return GUI.leftPanelSelectionX == 0;}

bool AtActionScreen() { return GUI.leftPanelSelectionY == 0 && GUI.leftPanelSelectionX == 1;}

bool AtLightScreen() { return GUI.leftPanelSelectionY == 1 && GUI.leftPanelSelectionX == 1;}

bool AtSensorScreen() { return GUI.leftPanelSelectionY == 2 && GUI.leftPanelSelectionX == 1;}

bool AtPumpScreen() { return GUI.leftPanelSelectionY == 3 && GUI.leftPanelSelectionX == 1;}

void LeftPanelUp() { GUI.leftPanelSelectionY = (GUI.leftPanelSelectionY <= 0 ? 0 : GUI.leftPanelSelectionY - 1); }

void LeftPanelDown() { GUI.leftPanelSelectionY = (GUI.leftPanelSelectionY >= 3 ? 3 : GUI.leftPanelSelectionY + 1); }

void LeftPanelBack() { GUI.leftPanelSelectionX = (GUI.leftPanelSelectionX <= 0 ? 0 : GUI.leftPanelSelectionX - 1); GUI.screenChanged = true; }

void LeftPanelEnter() { GUI.leftPanelSelectionX = (GUI.leftPanelSelectionX >= 3 ? 3 : GUI.leftPanelSelectionX + 1); GUI.screenChanged = true; }

