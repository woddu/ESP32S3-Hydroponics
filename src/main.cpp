#include "hydroponics.h"

#define LOG_RUNNING             "Log0"
#define LOG_PAUSED              "Log1"
#define LOG_WATER               "Log2"
#define LOG_FILL                "Log3"
#define LOG_FILL_CHECK          "Log4"
#define LOG_FILL_LOW            "Log5"
#define LOG_READ                "Log6"
#define LOG_DRAINING            "Log7"
#define LOG_LOW_PH              "Log8"
#define LOG_HIGH_PH             "Log9"
#define LOG_HIGH_PPM            "Log10"
#define LOG_LOW_PPM             "Log11"


#define SERVICE_UUID            "9ce8f60d-c116-42dd-852d-fd59028e84eb"
#define TDS_NOTIFY_UUID         "be0fc4c1-f772-4f20-9801-2ab78030a2a9"
#define LDR_NOTIFY_UUID         "be0fc4c2-f772-4f20-9801-2ab78030a2a9"
#define DATE_NOTIFY_UUID        "be0fc4c3-f772-4f20-9801-2ab78030a2a9"
#define HYDROSTATE_NOTIFY_UUID  "be0fc4c4-f772-4f20-9801-2ab78030a2a9"
#define LOGS_NOTIFY_UUID        "be0fc4c5-f772-4f20-9801-2ab78030a2a9"
#define TIMESTAMP_NOTIFY_UUID   "be0fc4c6-f772-4f20-9801-2ab78030a2a9"
#define LDHOUR_NOTIFY_UUID      "be0fc4c7-f772-4f20-9801-2ab78030a2a9"
// #define TESTSTATE_NOTIFY_UUID "be0fc4c7-f772-4f20-9801-2ab78030a2a9"

// #define RGB_WRITE_UUID          "b4a535a1-2a27-419c-8257-9cfcc67d6fd6"
#define DATE_WRITE_UUID         "b4a535a2-2a27-419c-8257-9cfcc67d6fd6"
#define ATTEMPT_WRITE_UUID      "b4a535a3-2a27-419c-8257-9cfcc67d6fd6"
#define RESET_WRITE_UUID        "b4a535a4-2a27-419c-8257-9cfcc67d6fd6"
#define LDHOUR_WRITE_UUID       "b4a535a5-2a27-419c-8257-9cfcc67d6fd6"

BLEServer *pServer = NULL;
BLECharacteristic *pNotifyTDS_pH = NULL;
BLECharacteristic *pNotifyLight_DHT = NULL;
BLECharacteristic *pNotifyDateSynced = NULL;
BLECharacteristic *pNotifyHydroponicState = NULL;
BLECharacteristic *pNotifyTimeStamp = NULL;
BLECharacteristic *pNotifyLDHour = NULL;
// BLECharacteristic *pNotifyTestLog = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

String ldHourString;

class MyServerCallbacks : public BLEServerCallbacks{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

class DateCallBack : public BLECharacteristicCallbacks{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        char myArray[rxValue.size() + 1]; // as 1 char space for null is also required
        strcpy(myArray, rxValue.c_str());
        if (rxValue.length() > 0)
        {
            try {
                unsigned int t = std::stoul(rxValue);
                struct timeval now = { .tv_sec = t }; 
                settimeofday(&now, NULL); 
                pNotifyDateSynced->setValue("Synced");
                pNotifyDateSynced->notify();
                Serial0.println("Convertion Successful");
                dateSynced = true;
                // pref.begin("Hydroponics");
                // pref.putULong("SavedTime", t);
                // pref.end();
            } catch (const std::exception& e) {
                // notify Date Synced
                pNotifyDateSynced->setValue("Not Synced");
                pNotifyDateSynced->notify();
                Serial0.println("Convertion Unsuccessful");
            }
            Serial0.println(rxValue.c_str());
        }
    };
};

class AttemptCallBack : public BLECharacteristicCallbacks{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        char myArray[rxValue.size() + 1]; // as 1 char space for null is also required
        strcpy(myArray, rxValue.c_str());
        if (rxValue.length() > 0)
        {
            if (rxValue == "start"){
                Serial0.println("Attempting to Start");
                startAttempt = true;
            } else if (rxValue == "pause"){
                Serial0.println("Attempting to Pause");
                pauseAttempt = true;
            } else if (rxValue == "drain"){
                Serial0.println("Attempting to Drain");
                if(Draining()){
                    draining = false;
                    GUI.screenChanged = true;
                    xTaskNotifyGive(xTFTChildPanelHandle);
                    pNotifyLogs->setValue(LOG_DRAINING_STOP);
                    pNotifyLogs->notify();
                } else {
                    drainAttempt = true;
                }
            } 
            Serial0.println(rxValue.c_str());
        }
    };
};

class ResetCallBack : public BLECharacteristicCallbacks{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        char myArray[rxValue.size() + 1]; // as 1 char space for null is also required
        strcpy(myArray, rxValue.c_str());
        if (rxValue.length() > 0)
        {
            if (rxValue == "reset"){

            }
            Serial0.println(rxValue.c_str());
        }
    };
};

class LDHourCallBack : public BLECharacteristicCallbacks{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        char myArray[rxValue.size() + 1]; // as 1 char space for null is also required
        strcpy(myArray, rxValue.c_str());
        if (rxValue.length() > 0)
        {
            ldHourString.remove(0);
            if (rxValue == "L+"){
                lightHour = (lightHour >= 24 ? 24 : lightHour + 1);
                darkHour = 24 - lightHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                xTaskNotifyGive(xTFTChildPanelHandle);
            } else if (rxValue == "L-"){
                lightHour = (lightHour <= 12 ? 12 : lightHour - 1);
                darkHour = 24 - lightHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                xTaskNotifyGive(xTFTChildPanelHandle);
            } else if (rxValue == "D+"){
                darkHour = (darkHour >= 12 ? 12 : darkHour + 1);
                lightHour = 24 - darkHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                xTaskNotifyGive(xTFTChildPanelHandle);
            } else if (rxValue == "D-"){
                darkHour = (darkHour <= 0 ? 0 : darkHour - 1);
                lightHour = 24 - darkHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                xTaskNotifyGive(xTFTChildPanelHandle);
            }
            Serial0.println(rxValue.c_str());
            vTaskDelay(75 / portTICK_PERIOD_MS);
        }
    };
};

void DHTLDR(void * param){
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);
    uint32_t delayMS = sensor.min_delay / 1000;
    sensors_event_t event;

    pinMode(LDR1_PIN, INPUT);
    pinMode(LDR2_PIN, INPUT);
    pinMode(LDR3_PIN, INPUT);
    pinMode(LDR4_PIN, INPUT);

    int ldr1, ldr2, ldr3, ldr4;

    while(1){

        ldr1 = analogRead(LDR1_PIN);
        ldr2 = analogRead(LDR2_PIN);
        ldr3 = analogRead(LDR3_PIN);
        ldr4 = analogRead(LDR4_PIN);

        Serial0.printf("ldr1: %d, ldr2: %d, ldr3: %d, ldr4: %d\n", ldr1, ldr2, ldr3, ldr4);

        // ldr0, ldr2 and ldr1, ldr3

        dht.temperature().getEvent(&event);
        if (isnan(event.temperature)) {
            Serial.println(F("Error reading temperature!"));
        } else {
            dhtTemp = event.temperature;
        }
        // Get humidity event and print its value.
        dht.humidity().getEvent(&event);
        if (isnan(event.relative_humidity)) {
            Serial0.println(F("Error reading humidity!"));
        } else {
            dhtHumid = event.relative_humidity;
        }
            
        vTaskDelay(delayMS / portTICK_PERIOD_MS);

    }
}

void DateandTime(void * param){
    bool switchCounting = false;
    unsigned long currentTime;
    while(1){
        currentTime = time(nullptr);
        if(Started()){
            runtimeTimeStamp = currentTime - dateStarted - pausedTime;
            if(countingLight){
                if(!switchCounting){
                    lightDarkCounter = runtimeTimeStamp;
                    switchCounting = true;
                }
                if(lightDarkCounter >= LightHour()){
                    countingLight = false;
                    Serial0.printf("lightHour Done\n");
                }
            } else {
                if(switchCounting){
                    lightDarkCounter = runtimeTimeStamp;
                    switchCounting = false;
                }
                if(lightDarkCounter >= DarkHour()){
                    countingLight = true;
                    Serial0.printf("DarkHour Done\n");
                }
            }
        } if (Paused()){
            pausedTime = currentTime - pausedStartTime;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void ServoLEDHumidifier(void * param){

    pinMode(GROW_LED, OUTPUT);
    pinMode(HUMIDIFIER, OUTPUT);
    int currentAngle = 0;
    int stepSize = 1;
    bool movingForward = true;
    unsigned long prevServoMillis;

    bool state;
    unsigned long stateInterval = 1000UL;
    while(1){

        if(Started()){
            
            if(dhtHumid < 40){ //HUMIDIty
                digitalWrite(HUMIDIFIER, HIGH);humidifierState = true;
            } else {
                digitalWrite(HUMIDIFIER, LOW);humidifierState = false;
            }

            if(countingLight){
                digitalWrite(GROW_LED, HIGH);growLightState = true;
                servoState = true;
                stateInterval = (!state ? 1750UL: 10UL);
                if (state){
                    stateInterval = (movingForward ? 30UL : 10UL);
                }
                if(millis() - prevServoMillis >= stateInterval){
                    prevServoMillis = millis();
                    if(movingForward){
                        if(!state){
                            servo.write(98);
                            currentAngle++;
                            Serial0.printf("Moving Forward, Angle: %d\n", currentAngle);
                        } else {
                            servo.write(90);
                        }
                        if (currentAngle >= 100) movingForward = false; 
                        state = !state;
                    } else {
                        if(!state){
                            servo.write(80);
                            currentAngle--;
                            Serial0.printf("Moving BackWard, Angle: %d\n", currentAngle);
                        } else {
                            servo.write(90);
                        }
                        if (currentAngle <= -25){
                            movingForward = true;
                            currentAngle = 0;
                        }
                        state = !state;
                    }
                }
                // if(millis() - prevServoMillis >= 200){
                //     prevServoMillis = millis();
                //     if (movingForward) {
                //         currentAngle += stepSize;
                //         if (currentAngle >= 180) { 
                //             movingForward = false;
                //         }
                //     } else {
                //         currentAngle -= stepSize;
                //         if (currentAngle <= 0) { 
                //             movingForward = true;
                //         }
                //     }           
                //     servo.write(currentAngle); 
                //     Serial0.printf("movingF: %d, currentAngle: %d,  \n", movingForward, currentAngle);
                // }       
            } else {
                digitalWrite(GROW_LED, LOW);growLightState = false;
                servo.write(90);servoState = false;
                // if(millis() - prevServoMillis >= 200){
                //     prevServoMillis = millis();
                //     if(currentAngle > 0){
                //         currentAngle -= stepSize;
                //         movingForward = true;
                //     }
                //     Serial0.printf("movingF: %d, currentAngle: %d,  \n", movingForward, currentAngle);
                // }  
            }
            
        } else if(Paused() || Draining()) {
            digitalWrite(GROW_LED, LOW);humidifierState = false;
            digitalWrite(HUMIDIFIER, LOW);growLightState = false;
            servo.write(90);servoState = false;
        } else if(NotRunning()) {
            servo.write(90);servoState = false;
            digitalWrite(GROW_LED, LOW);humidifierState = false;
            digitalWrite(HUMIDIFIER, LOW);growLightState = false;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

bool waterLeveltooLow;
void Pumps(void * param){

    pinMode(WATER_PUMP, OUTPUT);
    pinMode(WATER_DRAIN, OUTPUT);
    digitalWrite(WATER_DRAIN, HIGH);
    pinMode(AIR_PUMP, OUTPUT);

    while(1){
        if(Started()){
            if(!waterLeveltooLow){
                analogWrite(WATER_PUMP, 210);waterPumpState = true;
                digitalWrite(AIR_PUMP, HIGH);airPumpState = true;
                digitalWrite(WATER_DRAIN, HIGH);drainPumpState = false;
            } else {
                digitalWrite(AIR_PUMP, LOW);airPumpState = true;
                digitalWrite(WATER_DRAIN, HIGH);drainPumpState = false;
                analogWrite(WATER_PUMP, 0);waterPumpState = false;
            }
        } else if(Paused()){
            analogWrite(WATER_PUMP, 0);waterPumpState = false;
            digitalWrite(AIR_PUMP, LOW);airPumpState = false;
            digitalWrite(WATER_DRAIN, HIGH);drainPumpState = false;
        } else if (Draining()){
            analogWrite(WATER_PUMP, 0);waterPumpState = false;
            digitalWrite(AIR_PUMP, LOW);airPumpState = false;
            digitalWrite(WATER_DRAIN, LOW);drainPumpState = true;
        } else if (NotRunning()){
            analogWrite(WATER_PUMP, 0);waterPumpState = false;
            digitalWrite(AIR_PUMP, LOW);airPumpState = false;
            digitalWrite(WATER_DRAIN, HIGH);drainPumpState = true;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void BLE(void *param){
    pinMode(RESERVOIR, OUTPUT);
    digitalWrite(RESERVOIR, LOW);

    String dateSyncedString;
    dateSyncedString.reserve(50);
    
    int warnings = 0;
    bool startCounting1 = false, startCounting2 = false;
    unsigned long prevBLEMillis, reserviorM, readingM;
    while(1){
        if(deviceConnected){
            vTaskDelay(200 / portTICK_PERIOD_MS);
            if(prevLightHour != lightHour){
                ldHourString.remove(0);
                prevLightHour = lightHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
            } else if(prevDarkHour != darkHour){
                ldHourString.remove(0);
                prevDarkHour = darkHour;
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
            }

            if(Started()){
                if(millis() - prevBLEMillis >= 800){
                    prevBLEMillis = millis();
                    
                    pNotifyTimeStamp->setValue(std::to_string(runtimeTimeStamp));
                    pNotifyTimeStamp->notify();
                }
                
            }
        }
        if (!deviceConnected && oldDeviceConnected) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            pServer->startAdvertising();
            Serial0.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }
        // connecting
        if (deviceConnected && !oldDeviceConnected) {
            // do stuff here on connecting
            Serial0.println("Connecting...");
            oldDeviceConnected = deviceConnected;
            if(Started()){
                vTaskDelay(1500 / portTICK_PERIOD_MS);
                pNotifyHydroponicState->setValue("Running");
                pNotifyHydroponicState->notify();
                pNotifyLogs->setValue(LOG_RUNNING);
                pNotifyLogs->notify();
                
                dateSyncedString += (dateSynced ? "Synced," : "Not Synced,");
                dateSyncedString += String(dateStarted);
                pNotifyDateSynced->setValue(dateSyncedString.c_str());
                pNotifyDateSynced->notify();
                dateSyncedString.remove(0);

                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                vTaskDelay(75 / portTICK_PERIOD_MS);
            } else if(Paused()){
                vTaskDelay(1500 / portTICK_PERIOD_MS);
                pNotifyHydroponicState->setValue("Paused");
                pNotifyHydroponicState->notify();
                pNotifyLogs->setValue(LOG_PAUSED);
                pNotifyLogs->notify();
                
                dateSyncedString += (dateSynced ? "Synced," : "Not Synced,");
                dateSyncedString += String(dateStarted);
                pNotifyDateSynced->setValue(dateSyncedString.c_str());
                pNotifyDateSynced->notify();
                dateSyncedString.remove(0);

                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                vTaskDelay(75 / portTICK_PERIOD_MS);
            } else if(NotRunning()){

                vTaskDelay(1500 / portTICK_PERIOD_MS);
                pNotifyHydroponicState->setValue("Not Running");
                pNotifyHydroponicState->notify();

                dateSyncedString += (dateSynced ? "Synced" : "Not Synced");
                pNotifyDateSynced->setValue(dateSyncedString.c_str());
                pNotifyDateSynced->notify();
                dateSyncedString.remove(0);
                
                
                ldHourString += String(lightHour);
                ldHourString += ",";
                ldHourString += String(darkHour);
                pNotifyLDHour->setValue(ldHourString.c_str());
                pNotifyLDHour->notify();
                ldHourString.remove(0);
                vTaskDelay(75 / portTICK_PERIOD_MS);
            }
        }

        if(Starting()){
            if (waterLevel < 30){
                if(!startCounting1){
                    reserviorM = millis();
                    startCounting1 = true;
                    startCounting2 = false;
                    Serial0.println("Start Filling");
                    phRead = 0;
                    GUI.screenChanged = true;
                    xTaskNotifyGive(xTFTChildPanelHandle);
                }
                digitalWrite(RESERVOIR, HIGH);
                pNotifyLogs->setValue(LOG_FILL);
                pNotifyLogs->notify();
                prevBLEMillis = millis();
                if(millis() - reserviorM >= 60000){
                    reserviorM = millis();
                    if(warnings == 0){
                        pNotifyLogs->setValue(LOG_FILL_CHECK);
                        pNotifyLogs->notify();
                        warnings++;
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                    } else if (warnings == 1){
                        pNotifyLogs->setValue(LOG_FILL_LOW);
                        pNotifyLogs->notify();
                        warnings++;
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                    }  else if (warnings == 2){
                        pNotifyLogs->setValue(LOG_FILL_LOW);
                        pNotifyLogs->notify();

                        warnings = 0;
                        startAttempt = false;
                        GUI.screenChanged = true;
                    }
                }
            } else {
                if(!startCounting2){
                    Serial0.println("Done Filling");
                    Serial0.println("Start Reading");
                    readingM = millis();
                    startCounting1 = false;
                    startCounting2 = true;
                    phRead = 1;
                    GUI.screenChanged = true;
                    xTaskNotifyGive(xTFTChildPanelHandle);
                }
                digitalWrite(RESERVOIR, LOW);
                pNotifyLogs->setValue(LOG_READ);
                pNotifyLogs->notify();
                if (millis() - readingM >= 5000){
                    Serial0.println("Done Reading");
                    if(pH_Val < 5.4){
                        pNotifyLogs->setValue(LOG_LOW_PH);
                        pNotifyLogs->notify();
                        phRead = 2;
                        GUI.screenChanged = true;
                        xTaskNotifyGive(xTFTChildPanelHandle);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        startAttempt = false;
                    } else if(pH_Val > 6.6){
                        pNotifyLogs->setValue(LOG_HIGH_PH);
                        pNotifyLogs->notify();
                        phRead = 3;
                        GUI.screenChanged = true;
                        xTaskNotifyGive(xTFTChildPanelHandle);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        startAttempt = false;
                    } else {
                        
                        phRead = 0;
                        startAttempt = false;
                        prepared = true;
                        startCounting2 = false;
                        
                        GUI.screenChanged = true;
                        xTaskNotifyGive(xTFTChildPanelHandle);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                    }
                }
                    
                
            }
            
        } else if(Prepared()){
            started = true;
            dateStarted = time(nullptr);
            // pref.begin("Hydroponics");
            // pref.putInt("Started", started);
            // pref.putULong("DateStarted", dateStarted);
            // pref.end();
            Serial0.println(dateStarted);
            pNotifyHydroponicState->setValue("Running");
            pNotifyHydroponicState->notify();
            pNotifyLogs->setValue(LOG_RUNNING);
            pNotifyLogs->notify();
            dateSyncedString += (dateSynced ? "Synced," : "Not Synced,");
            dateSyncedString += String(dateStarted);
            pNotifyDateSynced->setValue(dateSyncedString.c_str());
            pNotifyDateSynced->notify();
            dateSyncedString.remove(0);

            

            GUI.screenChanged = true;
            vTaskDelay(75 / portTICK_PERIOD_MS);

        } else if(startAttempt && Paused()){
            pNotifyHydroponicState->setValue("Running");
            pNotifyHydroponicState->notify();
            pNotifyLogs->setValue(LOG_RUNNING);
            pNotifyLogs->notify();
            paused = false;
            startAttempt = false;

            GUI.screenChanged = true;
            
            vTaskDelay(75 / portTICK_PERIOD_MS);
        } else if (pauseAttempt && Started()){
            pNotifyHydroponicState->setValue("Paused");
            pNotifyHydroponicState->notify();
            pNotifyLogs->setValue(LOG_PAUSED);
            pNotifyLogs->notify();
            paused = true;
            pauseAttempt = false;
            pausedStartTime = time(nullptr);
            GUI.screenChanged = true;
            vTaskDelay(75 / portTICK_PERIOD_MS);
        } else if(drainAttempt){
            pNotifyHydroponicState->setValue("Draining");
            pNotifyHydroponicState->notify();
            pNotifyLogs->setValue(LOG_DRAINING);
            pNotifyLogs->notify();
            draining = true;
            drainAttempt = false;

            GUI.screenChanged = true;
            
            vTaskDelay(75 / portTICK_PERIOD_MS);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void TDSandPH(void *param){
    pinMode(TDS_TOGGLE, OUTPUT);
    String bleValue;
    bleValue.reserve(30);
    unsigned long prevMillis;

    int16_t adc0, adc1;
    float volt0, volt1;
    while (1)
    {
        digitalWrite(TDS_TOGGLE, HIGH);

        vTaskDelay(80 / portTICK_PERIOD_MS);
        if(xSemaphoreTake(xADSMutex, portMAX_DELAY) == pdTRUE){
            analogBuffer[analogBufferIndex] = ads.readADC_SingleEnded(1); // read the analog value and store into the buffer
            xSemaphoreGive(xADSMutex);
        }
        analogBufferIndex++;
        if (analogBufferIndex == SCOUNT)
        {
            analogBufferIndex = 0;
            for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
            {
                analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
            }
            // read the analog value more stable by the median filtering algorithm, and convert to voltage value
            // averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
            // Serial0.printf("median: %d, volts: %.2f, manual volts: %.2f\n", getMedianNum(analogBufferTemp, SCOUNT), ads.computeVolts(getMedianNum(analogBufferTemp, SCOUNT)), (getMedianNum(analogBufferTemp, SCOUNT) *3.3/ 17545));
            averageVoltage = ads.computeVolts(getMedianNum(analogBufferTemp, SCOUNT));
            // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
            // float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
            float compensationCoefficient = 1.0 + 0.02 * (dhtTemp - 25.0);
            // temperature compensation
            float compensationVoltage = averageVoltage / compensationCoefficient;

            // convert voltage value to tds value

            tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
        }
        digitalWrite(TDS_TOGGLE, LOW);

        vTaskDelay(80 / portTICK_PERIOD_MS);
        float total = 0;

        for(int i = 0; i < 30; i++){
            if(xSemaphoreTake(xADSMutex, portMAX_DELAY) == pdTRUE){
                adc0 = ads.readADC_SingleEnded(0);
                volt0 = ads.computeVolts(adc0);
                xSemaphoreGive(xADSMutex);
            }
            total += m * volt0 + b;
            vTaskDelay(2 / portTICK_PERIOD_MS);
        }


        pH_Val = total / 30.0;
        
        lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
        if (measure.RangeStatus != 4) {  // phase failures have incorrect data
            waterLevel = map(measure.RangeMilliMeter / 10, 13, 23, 100, 0);
            waterLevel = (waterLevel < 0 ? 0 : waterLevel);
        } else {
            waterLevel = 0;
        }
        if (millis() - prevMillis >= 1500)
        {
            prevMillis = millis();
            
            // Serial0.printf("a0: %d, a1: %d, a2: %d, a3: %d\n", adc0, ads.readADC_SingleEnded(1), LDRs.readADC_SingleEnded(0), LDRs.readADC_SingleEnded(1));
            // Serial0.printf("A2: %d, Volts: %.4f\n", ads2, volt2);
            // Serial0.printf("A3: %d, Volts: %.4f\n", ads3, volt3);
            if (measure.RangeStatus != 4) {  // phase failures have incorrect data
                Serial0.printf("Distance (mm): %d\n", measure.RangeMilliMeter); 
            } else {
                Serial0.println(" out of range ");
            }
            if (deviceConnected)
            {
                bleValue += String(tdsValue, 1);
                bleValue += ",";
                bleValue += String(pH_Val, 1);
                bleValue += ",";
                if (measure.RangeStatus != 4) {
                    bleValue += String(waterLevel);
                    // Serial0.printf("Distance: %d, Level: %d", measure.RangeMilliMeter, waterLevel);
                    
                } else {
                    bleValue += String(0);
                    Serial0.println(" out of range ");
                }
                // Serial0.println(bleValue);
                bleValue += ",";
                bleValue += String(dhtTemp, 1);
                bleValue += ",";
                bleValue += String(dhtHumid, 1);
                pNotifyTDS_pH->setValue(bleValue.c_str());
                pNotifyTDS_pH->notify();
                bleValue.remove(0);
            }
        }
    }
}

void setup(){

    servo.attach(SERVO_PIN);
    servo.write(90);

    Serial0.begin(115200);
  
    ldHourString.reserve(50);

    xTFTPrintMutex = xSemaphoreCreateMutex();
    if (xTFTPrintMutex == NULL){
        Serial0.println("Mutex Creation Failed");
    } else {
        Serial0.println("Mutex Creation Succesful");
    }
    xADSMutex = xSemaphoreCreateMutex();
    if (xTFTPrintMutex == NULL){
        Serial0.println("Mutex Creation Failed");
    } else {
        Serial0.println("Mutex Creation Succesful");
    }

    tft.init(240, 320);

    tft.invertDisplay(false);

    tft.setRotation(3);
    tft.setTextWrap(false);
    tft.setFont(&FreeSans12pt7b);

    tft.fillScreen(surface);
    tft.fillRect(0,0, 80, 240, surfaceVariant);
    tft.fillRect(0,0, 320, 60, primary);

    tft.setTextSize(2);
    tft.getTextBounds("iQuaRoots", 0, 0, &xBuf, &yBuf, &wBuf, &hBuf);
    TFTPrint((tft.width() - wBuf) / 2, ((60 - hBuf) / 2) + 6, "iQuaRoots", 0xFFFF, 2);
    
    RectWithCenteredText(true, 0, 60, 80, 37, secondary, "Actions", 0xFFFF);
    
    RectWithCenteredText(true, 0, 97, 80, 37, surfaceVariant, "Light", onSurfaceVariant);
    
    RectWithCenteredText(true, 0, (134), 80, 37, surfaceVariant, "Sensor", onSurfaceVariant);
    
    RectWithCenteredText(true, 0, (171), 80, 37, surfaceVariant, "Pump..", onSurfaceVariant);

    BLEDevice::init("Hyrdoponics");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 40);

    pNotifyTDS_pH = pService->createCharacteristic(
        TDS_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyLight_DHT = pService->createCharacteristic(
        LDR_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyDateSynced = pService->createCharacteristic(
        DATE_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyHydroponicState = pService->createCharacteristic(
        HYDROSTATE_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyLogs = pService->createCharacteristic(
        LOGS_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyTimeStamp = pService->createCharacteristic(
        TIMESTAMP_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    pNotifyLDHour = pService->createCharacteristic(
        LDHOUR_NOTIFY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );

    BLE2902 *pBLE2902_1 = new BLE2902();
    pBLE2902_1->setNotifications(true);
    pNotifyTDS_pH->addDescriptor(pBLE2902_1);
    BLE2902 *pBLE2902_2 = new BLE2902();
    pBLE2902_2->setNotifications(true);
    pNotifyLight_DHT->addDescriptor(pBLE2902_2);
    BLE2902 *pBLE2902_3 = new BLE2902();
    pBLE2902_3->setNotifications(true);
    pNotifyDateSynced->addDescriptor(pBLE2902_3);
    BLE2902 *pBLE2902_4 = new BLE2902();
    pBLE2902_4->setNotifications(true);
    pNotifyHydroponicState->addDescriptor(pBLE2902_4);
    BLE2902 *pBLE2902_5 = new BLE2902();
    pBLE2902_5->setNotifications(true);
    pNotifyLogs->addDescriptor(pBLE2902_5);
    BLE2902 *pBLE2902_6 = new BLE2902();
    pBLE2902_6->setNotifications(true);
    pNotifyTimeStamp->addDescriptor(pBLE2902_6);
    BLE2902 *pBLE2902_7 = new BLE2902();
    pBLE2902_7->setNotifications(true);
    pNotifyLDHour->addDescriptor(pBLE2902_7);

    BLECharacteristic * pDateWrite = pService->createCharacteristic(
        DATE_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pDateWrite->setCallbacks(new DateCallBack());
    
    BLECharacteristic * pAttemptWrite = pService->createCharacteristic(
        ATTEMPT_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pAttemptWrite->setCallbacks(new AttemptCallBack());
    
    BLECharacteristic * pResetWrite = pService->createCharacteristic(
        RESET_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pResetWrite->setCallbacks(new ResetCallBack());
    
    BLECharacteristic * pLDHourWrite = pService->createCharacteristic(
        LDHOUR_WRITE_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pLDHourWrite->setCallbacks(new LDHourCallBack());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();

    // if (!ads.begin(0x48)) {
    //     Serial0.println("Failed to initialize ads.");
    //     while (1);
    // }
    if (!ads.begin(0x49)) {
        Serial0.println("Failed to initialize adss.");
        while (1);
    }
    if (!lox.begin(0x29)) {
        Serial0.println(F("Failed to boot VL53L0X"));
        while(1);
    }

    dht.begin();

    phCalibration();

    /*
        with ads1115
        ads1115 resolution
        <= 8
        >= 17540 
        m = (4.01 - 6.86) / (3.1024 - 2.62);

        b = 6.86 - m * 2.62;
    */


    xTaskCreatePinnedToCore(
        TDSandPH,
        "TDS and PH",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        Pumps,
        "Pumps",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        BLE,
        "BLE",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        ServoLEDHumidifier,
        "Servo LED Humidifier",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        DateandTime,
        "Date and Time",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        TFTLeftPanel,
        "TFT Left Panel",
        4096,
        NULL,
        1,
        &xTFTLeftPanelHandle,
        1
    );

    xTaskCreatePinnedToCore(
        TFTChildPanel,
        "TFT Child Panel",
        4096,
        NULL,
        1,
        &xTFTChildPanelHandle,
        1
    );

    xTaskCreatePinnedToCore(
        Joystick,
        "Joystick",
        4096,
        NULL,
        2,
        NULL,
        0
    );
    
    xTaskCreatePinnedToCore(
        DHTLDR,
        "DHT LDR",
        4096,
        NULL,
        1,
        NULL,
        0
    );
    
}

void loop(){

    // prevX = currX;
    // prevY = currY;
    // currX = ads2;
    // currY = ads3;

    // if (millis() - prevMil2 >= 1000){
    //   prevMil2 = millis();

    //   Serial0.printf("x: %d, y: %d\n", currX, currY);
    // }

    // if(prevX <= 17000 && currX > 17000){  // Right
    //     if (AtMainScreen()){
    //       LeftPanelEnter();
          
    //     } else if (AtActionScreen()){
    //       // Actions
    //       if(GUI.actionSelectionY == 0){
    //         if(started){
    //           GUI.actionSelectionY == 0;
    //           LeftPanelBack();
    //           pauseAttempt = true;
    //         } else {
    //           // if there is water
    //           GUI.actionSelectionY == 0;
    //           LeftPanelBack();
    //           startAttempt = true;
    //         }
    //       } else {
    
    //       }
    //       xTaskNotifyGive(xTFTChildPanelHandle);
    //     } else if (AtLightScreen()){
    //       GUI.lightSelectionX = 1;
    //       xTaskNotifyGive(xTFTChildPanelHandle);
    //     } else if (AtSensorScreen()){
    
    //     }
    //   } else if(prevX >= 2000 && currX < 2000){ // Left
    //     if (AtActionScreen()){
    //       xTaskNotifyGive(xTFTChildPanelHandle);
    //       LeftPanelBack();
    //     } else if (AtLightScreen()){
    //       if (GUI.lightSelectionX == 0) {
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //         LeftPanelBack();
    //       } else if(GUI.lightSelectionX == 1){
    //         GUI.lightSelectionX = 0;
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //       }
    //     } else if (AtSensorScreen()){
    //       LeftPanelBack();
    //       xTaskNotifyGive(xTFTLeftPanelHandle);
    //     } else if (AtPumpScreen()){
    //       LeftPanelBack();
    //       xTaskNotifyGive(xTFTLeftPanelHandle);
    //     }
    //   }
  
    //   if(prevY <= 17000 && currY > 17000){// Down
    //     if (AtMainScreen()){
    //       LeftPanelDown();
    //       xTaskNotifyGive(xTFTLeftPanelHandle);
    //     } else if (AtActionScreen()){
    //       GUI.actionSelectionY = 1;
    //       xTaskNotifyGive(xTFTChildPanelHandle);
    //     } else if (AtLightScreen()){
    //       if (GUI.lightSelectionX == 0) {
    //         lightHour = (lightHour <= 12 ? 12 : lightHour - 1);
    //         darkHour = 24 - lightHour;
    //         // pref.begin("Hydroponics");
    //         // pref.putInt("LightHour", lightHour);
    //         // pref.putInt("DarkHour", darkHour);
    //         // pref.end();
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //       } else if(GUI.lightSelectionX == 1){
    //         darkHour = (darkHour <= 0 ? 0 : darkHour - 1);
    //         lightHour = 24 - darkHour;
    //         // pref.begin("Hydroponics");
    //         // pref.putInt("LightHour", lightHour);
    //         // pref.putInt("DarkHour", darkHour);
    //         // pref.end();
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //       }
    //     } else if (AtSensorScreen()){
          
    //     } else if (AtPumpScreen()){
          
    //     }
    //   } else if(prevY >= 2000 && currY < 2000){// Up
    //     if (AtMainScreen()){
    //       LeftPanelUp();
    //       xTaskNotifyGive(xTFTLeftPanelHandle);
    //     } else if (AtActionScreen()){
    //       GUI.actionSelectionY = 0;
    //       xTaskNotifyGive(xTFTChildPanelHandle);
    //     } else if (AtLightScreen()){
    //       if (GUI.lightSelectionX == 0) {
    //         lightHour = (lightHour >= 24 ? 24 : lightHour + 1);
    //         darkHour = 24 - lightHour;
    //         // pref.begin("Hydroponics");
    //         // pref.putInt("LightHour", lightHour);
    //         // pref.putInt("DarkHour", darkHour);
    //         // pref.end();
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //       } else if(GUI.lightSelectionX == 1){
    //         darkHour = (darkHour >= 12 ? 12 : darkHour + 1);
    //         lightHour = 24 - darkHour;
    //         // pref.begin("Hydroponics");
    //         // pref.putInt("LightHour", lightHour);
    //         // pref.putInt("DarkHour", darkHour);
    //         // pref.end();
    //         xTaskNotifyGive(xTFTChildPanelHandle);
    //       }
    //     } else if (AtSensorScreen()){
          
    //     } else if (AtPumpScreen()){
          
    //     }
    //   }
    // }

    vTaskDelay(100 / portTICK_PERIOD_MS);
}

