//file: main.cpp

#include <Arduino.h>
#include "DEVICE_ID.c"
#include <NimBLEDevice.h>
#include "SerialAndTFT.h"
#include "BLEMesh.h"

int tDly = 3000; // Delay in milliseconds
int tInterval = 300; // Scan interval in milliseconds
int tWindow = 200; // Scan window in milliseconds



void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    // Wait for serial monitor to open
    tft.begin();
    delay(1000);
    // Print startup message
    printStartupMessage();  

    // Initialize BLE
    NimBLEDevice::init("ESP32-S3_BLE_MESH");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    //wait for initialization
    delay(1000);
    
// Start advertising
    startAdvertising();
}

ScanCallback scanCallback;

void loop() {
    // Reset the RSSI values matrix
    resetRSSIValues();

    // Start BLE scan
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCallback);
    pScan->setActiveScan(true);
    pScan->setInterval(tInterval);
    pScan->setWindow(tWindow);
    pScan->start(0, nullptr);

    // Update advertising data with new RSSI values
    startAdvertising();

    // Print RSSI values to the Serial Monitor
    printRSSIValues();
    delay(tDly);
}