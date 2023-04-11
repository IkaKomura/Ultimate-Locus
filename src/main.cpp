#include <Arduino.h>
#include "DEVICE_ID.c"
#include <NimBLEDevice.h>
//#include <TFT_eSPI.h>
#include "SerialAndTFT.h"

int tDly = 3000; // Delay in milliseconds
int tInterval = 300; // Scan interval in milliseconds
int tWindow = 200; // Scan window in milliseconds

TFT_eSPI tft = TFT_eSPI();
SerialAndTFT output(tft);

//////////////// Function prototypes/////////////////////////////

void printStartupMessage();
void resetRSSIValues();
void storeRSSI(uint8_t deviceId, int rssi);
void startMessageAdvertising();
void startAdvertising();
uint8_t countConnectedDevices(uint8_t deviceId);
void printRSSIValues();
void printReceivedMessage(const std::string &message);


// Unique device ID
const uint8_t DEVICE_ID = device_ID; // Make sure to set a unique ID for each device

// BLE UUIDs
#define SERVICE_UUID "0000ABCD-0000-1000-8000-00805F9B34FB"

// Maximum number of devices
#define MAX_DEVICES 10

// RSSI values matrix
int rssiValues[MAX_DEVICES][MAX_DEVICES] = {0};

void printStartupMessage() {
    Serial.println("=====================================");
    Serial.println("       ESP32 BLE Mesh Device");
    Serial.print("       Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.println("=====================================");
    Serial.println();
}

void resetRSSIValues() {
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        for (uint8_t j = 0; j < MAX_DEVICES; j++) {
            rssiValues[i][j] = 0;
        }
    }
}

// Store RSSI value for a device
void storeRSSI(uint8_t deviceId, int rssi) {
    if (deviceId < MAX_DEVICES) {
        rssiValues[DEVICE_ID][deviceId] = rssi;
    }
}

// Advertise a test message
void startMessageAdvertising() {
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advData;

    advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advData.setCompleteServices(NimBLEUUID(SERVICE_UUID));

    std::string message = "Hello, this is a test message!";
    advData.setManufacturerData(message);

    // Debugging: Print the manufacturer data being set
    Serial.print("Setting manufacturer data for advertising: ");
    Serial.println(message.c_str());

    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
}

// Advertise device ID, service UUID, and RSSI values for other nodes
void startAdvertising() {
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advData;

    advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advData.setCompleteServices(NimBLEUUID(SERVICE_UUID));

    std::string manufData(1 + MAX_DEVICES * 2, 0); // Allocate space for device ID and RSSI values
    manufData[0] = DEVICE_ID;

    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (rssiValues[DEVICE_ID][i] != 0) {
            manufData[1 + i * 2] = i;
            manufData[2 + i * 2] = rssiValues[DEVICE_ID][i];
        }
    }

    advData.setManufacturerData(manufData);
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
}

// Scan callback class
class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.println("Device found during scan...");

        if (advertisedDevice->haveServiceUUID()) {
            Serial.print("  Advertised Service UUID: ");
            Serial.println(advertisedDevice->getServiceUUID().toString().c_str());
        }

        if (advertisedDevice->getManufacturerData().length() > 0) {
            Serial.print("  Manufacturer data: ");
            Serial.println(advertisedDevice->getManufacturerData().c_str());
        }

        // Check if the advertised device is one of our mesh devices
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            std::string manufData = advertisedDevice->getManufacturerData();

            // Debugging: print the first 30 characters of the manufacturer data
            Serial.print("  First 30 chars of manufData: ");
            Serial.println(manufData.substr(0, 30).c_str());

            // Debugging: print the expected message substring
            Serial.println("  Expected message substring: Hello, this is a test message!");

            // Check if the advertised data contains the modified message
            if (manufData.substr(0, 30) == "Hello, this is a test message!") {
                printReceivedMessage(manufData);
            } else {
                Serial.println("Device is not advertising the message...");
            }

            if (manufData.length() > 0 && manufData[0] != DEVICE_ID) {
                Serial.println("Device is advertising the correct service UUID...");
                uint8_t id = manufData[0];
                int rssi = advertisedDevice->getRSSI();

                // Store the RSSI value for the detected device
                storeRSSI(id, rssi);

                // Parse advertised data to get RSSI values from the detected device
                for (size_t i = 1; i < manufData.size(); i += 2) {
                    uint8_t deviceId = manufData[i];
                    int deviceRssi = (int8_t)manufData[i + 1];
                    rssiValues[deviceId][DEVICE_ID] = deviceRssi;
                }
            }
        } else {
            Serial.println("Device is not advertising the correct service UUID...");
            return;
        }
    }
};

uint8_t countConnectedDevices(uint8_t deviceId) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (rssiValues[deviceId][i] != 0) {
            count++;
        }
    }
    return count;
}

// Print RSSI values to the Serial Monitor
void printRSSIValues() {
    bool hasConnections = false;

    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        uint8_t connectedDevices = countConnectedDevices(i);
        if (connectedDevices > 0) {
            hasConnections = true;
            Serial.print("Device ");
            Serial.print(i);
            Serial.print(" is connected to ");
            Serial.print(connectedDevices);
            Serial.println(" devices.");
        }
        for (uint8_t j = 0; j < MAX_DEVICES; j++) {
            if (rssiValues[i][j] != 0) {
                Serial.print("  Device ");
                Serial.print(i);
                Serial.print(" -> ");
                Serial.print("Device ");
                Serial.print(j);
                Serial.print(": ");
                Serial.println(rssiValues[i][j]);
            }
        }
    }

    if (!hasConnections) {
        Serial.println("No available devices.");
    }

    Serial.println();
}
// Print test message to the Serial Monitor
void printReceivedMessage(const std::string &message) {
    Serial.print("  Received message: ");
    Serial.println(message.c_str());
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    // Wait for serial monitor to open
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