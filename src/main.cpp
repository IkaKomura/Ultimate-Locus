#include <Arduino.h>

#include <NimBLEDevice.h>

// Unique device ID
const uint8_t DEVICE_ID = 2; // Make sure to set a unique ID for each device

// BLE UUIDs
#define SERVICE_UUID "0000ABCD-0000-1000-8000-00805F9B34FB"

// Maximum number of devices
#define MAX_DEVICES 10

// RSSI values matrix
int rssiValues[MAX_DEVICES][MAX_DEVICES] = {0};

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



// Advertise device ID, service UUID, and RSSI values for other nodes
void startAdvertising() {
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData advData;

    advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    advData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
    advData.setManufacturerData(std::string((char *)&DEVICE_ID, 1));

    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
}

// Scan callback class
class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        // Check if the advertised device is one of our mesh devices
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            uint8_t id = advertisedDevice->getManufacturerData()[0];
            int rssi = advertisedDevice->getRSSI();

            // Store the RSSI value for the detected device
            storeRSSI(id, rssi);

            // Parse advertised data to get RSSI values from the detected device
            std::string advertisedData = advertisedDevice->getManufacturerData();
            for (size_t i = 1; i < advertisedData.size(); i += 2) {
                uint8_t deviceId = advertisedData[i];
                int deviceRssi = advertisedData[i + 1];
                rssiValues[deviceId][DEVICE_ID] = deviceRssi;
            }
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



void setup() {
    // Initialize serial communication
    Serial.begin(115200);

    // Initialize BLE
    NimBLEDevice::init("ESP32-S3_BLE_MESH");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Start advertising
    startAdvertising();
}
//
ScanCallback scanCallback;

void loop() {
    // Reset the RSSI values matrix
    resetRSSIValues();

    // Start BLE scan
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCallback);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->start(0, nullptr);

    // Print RSSI values to the Serial Monitor
    printRSSIValues();
}

