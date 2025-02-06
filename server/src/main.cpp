#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// HC-SR04 distance sensor pin setup
#define TRIG_PIN  4
#define ECHO_PIN  5

// BLE UUIDs
#define SERVICE_UUID        "90e3f8e8-6a54-4287-aad5-b3eaed884cd1"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// DSP Algorithm (Low-pass filter or moving average)
const int MOVING_AVERAGE_SIZE = 5;  // Size of the moving average
float distanceReadings[MOVING_AVERAGE_SIZE] = {0};  // Initialize to 0
int currentIndex = 0;  // Current index
float movingAverage = 0;

// Connection callback
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// Initialize the distance sensor
void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Initialize BLE device
    BLEDevice::init("XIAO_ESP32S3");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

// Read HC-SR04 distance sensor data
long readDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    long distance = (duration / 2) * 0.0343;  // Calculate distance (cm)
    return distance;
}

// Moving average DSP algorithm
float applyMovingAverage(float newReading) {
    // Add the new reading
    distanceReadings[currentIndex] = newReading;
    currentIndex = (currentIndex + 1) % MOVING_AVERAGE_SIZE;

    // Calculate moving average
    float sum = 0;
    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++) {
        sum += distanceReadings[i];
    }
    movingAverage = sum / MOVING_AVERAGE_SIZE;
    return movingAverage;
}

void loop() {
    long rawDistance = readDistance();  // Read raw distance data
    float denoisedDistance = applyMovingAverage(rawDistance);  // Apply moving average filter

    // Display raw data and denoised data
    Serial.print("Raw Distance: ");
    Serial.print(rawDistance);
    Serial.print(" cm, Denoised Distance: ");
    Serial.println(denoisedDistance);

    if (deviceConnected) {
        // Send new readings to BLE
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            pCharacteristic->setValue("Hello World");  // Update with processed data
            pCharacteristic->notify();
            Serial.println("Notify value: Hello World");
        }
    }
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the Bluetooth stack a chance to get things ready
        pServer->startAdvertising();  // Advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
        // Do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
