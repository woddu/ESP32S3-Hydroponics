#pragma once
float m;

float b;

int pHReading ;
float pH_Val; 
float averagepH_Value; 
float Voltage;

void phCalibration() {
    m = (4.01 - 6.86) / (3.1024 - 2.62);

    b = 6.86 - m * 2.62;
}