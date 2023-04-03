#include <Arduino.h>

#include <NimBLEDevice.h>

// Unique device ID
const uint8_t DEVICE_ID = 1; // Make sure to set a unique ID for each device

// BLE scan duration in seconds
const int SCAN_DURATION = 5;

// BLE UUIDs
#define SERVICE_UUID "0000ABCD-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "0000ABCD-0001-1000-8000-00805F9B34FB"

// Maximum number of devices
#define MAX_DEVICES 10

// RSSI values matrix
int rssiValues[MAX_DEVICES][MAX_DEVICES] = {0};

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

    // Add RSSI values to the advertisement data
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (rssiValues[DEVICE_ID][i] != 0) {
            advData.addData(std::string((char *)&i, 1) + std::string((char *)&rssiValues[DEVICE_ID][i], 1));
        }
    }

    pAdvertising->setAdvertisementData(advData);
    pAdvertising->start();
}

// Scan callback class
class ScanCallback : public NimBLEScanCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
        // Check if the advertised device is one of our mesh devices
        NimBLERemoteService *pService = advertisedDevice->getServiceByUUID(SERVICE_UUID);
        if (pService) {
            uint8_t id = pService->getCharacteristic(CHARACTERISTIC_UUID)->readUInt8();
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

// Print RSSI values to the Serial Monitor
void printRSSIValues() {
    Serial.println("RSSI values:");
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        for (uint8_t j = 0; j < MAX_DEVICES; j++) {
            if (rssiValues[i][j] != 0) {
                Serial.print("Device ");
                Serial.print(i);
                Serial.print(" -> ");
                Serial.print("Device ");
                Serial.print(j);
                Serial.print(": ");
                Serial.println(rssiValues[i][j]);
            }
        }
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

void loop() {
    // Start BLE scan
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new ScanCallback());
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->start(SCAN_DURATION, nullptr);

    // Print RSSI values to the Serial Monitor
    printRSSIValues();
}
