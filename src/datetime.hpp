#pragma once
#define HOUR 3600UL
#define DAY 86400UL

const char months[][12] = {"Jan","Feb","Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

const char weekDays[][7] = {"Sun","Mon","Tue", "Wed", "Thu", "Fri", "Sat"};

struct tm timeinfo;

unsigned long dateStarted;

unsigned long runtimeTimeStamp, pausedTime, pausedStartTime;

unsigned long savedTime;

bool dateSynced;
