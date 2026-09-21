#include "sensors.h"

void AHT30Sensor::init() {
    available = aht.begin();
}

void AHT30Sensor::readData() {
    if (!available || readState != AHT_IDLE) return;
    Wire.beginTransmission(0x38);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    if (Wire.endTransmission() == 0) {
        readDeadline = micros() + 80000;
        readState = AHT_WAITING;
    }
}

void AHT30Sensor::update() {
    if (readState != AHT_WAITING || static_cast<int32_t>(micros() - readDeadline) < 0) return;
    if (Wire.requestFrom(static_cast<uint8_t>(0x38), static_cast<uint8_t>(6)) == 6) {
        uint8_t frame[6];
        for (uint8_t i = 0; i < 6; ++i) frame[i] = static_cast<uint8_t>(Wire.read());

        // AHT30 returns status, humidity[19:0], temperature[19:0], CRC.
        uint32_t humidityRaw = (static_cast<uint32_t>(frame[1]) << 12) |
            (static_cast<uint32_t>(frame[2]) << 4) | (frame[3] >> 4);
        uint32_t temperatureRaw = (static_cast<uint32_t>(frame[3] & 0x0F) << 16) |
            (static_cast<uint32_t>(frame[4]) << 8) | frame[5];
        bool validFrame = (frame[0] & 0x80) == 0 &&
            humidityRaw <= 1048575UL && temperatureRaw <= 1048575UL;
        if (validFrame) {
            this->humidityRaw = humidityRaw;
            this->temperatureRaw = temperatureRaw;
            humidity = (humidityRaw * 100.0f) / 1048576.0f;
            temp = (temperatureRaw * 200.0f) / 1048576.0f - 50.0f;
        }
    }
    readState = AHT_IDLE;
}

String AHT30Sensor::getDisplayString(uint8_t index) {
    if (!available) return "N/A";
    if (index == 0) return String(temp, 1) + " C";
    return String(humidity, 1) + " %";
}

GP2Y1010AU0FSensor::GP2Y1010AU0FSensor(uint8_t led, uint8_t analog)
    : ledPin(led), analogPin(analog) {}

void GP2Y1010AU0FSensor::readData() {
    if (readState != DUST_IDLE) return;
    // Start the LED pulse; update() completes it without blocking the loop.
    digitalWrite(ledPin, HIGH);
    stateDeadline = micros() + 280;
    readState = DUST_SETTLE;
}

void GP2Y1010AU0FSensor::update() {
    uint32_t now = micros();
    if (readState == DUST_SETTLE && static_cast<int32_t>(now - stateDeadline) >= 0) {
        rawValue = analogRead(analogPin);
        int sample = rawValue;
        dustDensity = (sample * (3.3f / 4095.0f) - 0.6f) * 200.0f;
        if (dustDensity < 0) dustDensity = 0;
        // End the pulse and leave enough time for the next measurement.
        stateDeadline = now + 10000; // 9680;
        readState = DUST_COOLDOWN;
    } else if (readState == DUST_COOLDOWN && static_cast<int32_t>(now - stateDeadline) >= 0) {
        readState = DUST_IDLE;
    }
}

/*
 * The LED is turned off after the sample. The 9.68 ms cooldown is tracked by
 * micros() rather than delaying the main loop.
 */
void GP2Y1010AU0FSensor::init() {
    pinMode(ledPin, OUTPUT);
    pinMode(analogPin, INPUT);
    digitalWrite(ledPin, LOW);
}

String GP2Y1010AU0FSensor::getDisplayString(uint8_t index) {
    return String(dustDensity, 0) + " ug/m3";
}

int GP2Y1010AU0FSensor::getAqi() const {
    if (dustDensity <= 35) return map(dustDensity, 0, 35, 0, 50);
    if (dustDensity <= 75) return map(dustDensity, 35, 75, 51, 100);
    if (dustDensity <= 115) return map(dustDensity, 75, 115, 101, 150);
    if (dustDensity <= 150) return map(dustDensity, 115, 150, 151, 200);
    if (dustDensity <= 250) return map(dustDensity, 150, 250, 201, 300);
    return 300;
}

const char* GP2Y1010AU0FSensor::getQuality() const {
    if (dustDensity <= 35) return "Excellent";
    if (dustDensity <= 75) return "Average";
    if (dustDensity <= 115) return "Light pollution";
    if (dustDensity <= 150) return "Moderate pollution";
    if (dustDensity <= 250) return "Heavy pollution";
    return "Serious pollution";
}

MQ135Sensor::MQ135Sensor(uint8_t analog) : analogPin(analog) {}

void MQ135Sensor::init() {
    pinMode(analogPin, INPUT);
}

void MQ135Sensor::readData() {
    rawValue = analogRead(analogPin);
}

String MQ135Sensor::getDisplayString(uint8_t index) {
    return String(rawValue);
}

void SensorManager::addSensor(Sensor* sensor) {
    if (sensorCount < MAX_SENSORS) sensors[sensorCount++] = sensor;
}
void SensorManager::initAll() {
    for (uint8_t i = 0; i < sensorCount; ++i) sensors[i]->init();
}
void SensorManager::readAll() {
    for (uint8_t i = 0; i < sensorCount; ++i) sensors[i]->readData();
}
void SensorManager::updateAll() {
    for (uint8_t i = 0; i < sensorCount; ++i) sensors[i]->update();
}
Sensor* SensorManager::getSensor(size_t index) {
    if (index < sensorCount) return sensors[index];
    return nullptr;
}
size_t SensorManager::count() const { return sensorCount; }