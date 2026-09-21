#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Adafruit_AHTX0.h>

class Sensor {
public:
    virtual void init() = 0;
    virtual void readData() = 0;
    virtual void update() {}
    virtual String getDisplayString(uint8_t index = 0) = 0;
    virtual ~Sensor() {}
};

class AHT30Sensor : public Sensor {
private:
    enum ReadState : uint8_t { AHT_IDLE, AHT_WAITING };
    Adafruit_AHTX0 aht;
    float temp = 0.0;
    float humidity = 0.0;
    bool available = false;
    bool useFahrenheit = false;
    uint32_t temperatureRaw = 0;
    uint32_t humidityRaw = 0;
    ReadState readState = AHT_IDLE;
    uint32_t readDeadline = 0;
public:
    void init() override;
    void readData() override;
    void update() override;
    String getDisplayString(uint8_t index = 0) override;
    float getTemperature() const { return temp; }
    uint32_t getRawTemperature() const { return temperatureRaw; }
    uint32_t getRawHumidity() const { return humidityRaw; }
    float getDisplayedTemperature() const { return useFahrenheit ? (temp * 1.8f + 32.0f) : temp; }
    const char* getTemperatureUnit() const { return useFahrenheit ? " F" : " C"; }
    float getHumidity() const { return humidity; }
    bool isAvailable() const { return available; }
    void setFahrenheit(bool enabled) { useFahrenheit = enabled; }
};

class GP2Y1010AU0FSensor : public Sensor {
private:
    enum ReadState : uint8_t { DUST_IDLE, DUST_SETTLE, DUST_COOLDOWN };
    uint8_t ledPin;
    uint8_t analogPin;
    ReadState readState = DUST_IDLE;
    uint32_t stateDeadline = 0;
    float dustDensity = 0.0;
    int rawValue = 0;
public:
    GP2Y1010AU0FSensor(uint8_t led, uint8_t analog);
    void init() override;
    void readData() override;
    void update() override;
    String getDisplayString(uint8_t index = 0) override;
    float getDensity() const { return dustDensity; }
    int getAqi() const;
    const char* getQuality() const;
    int getRawValue() const { return rawValue; }
};

class MQ135Sensor : public Sensor {
private:
    uint8_t analogPin;
    int rawValue = 0;
public:
    MQ135Sensor(uint8_t analog);
    void init() override;
    void readData() override;
    String getDisplayString(uint8_t index = 0) override;
    int getRawValue() const { return rawValue; }
};

class SensorManager {
private:
    static const uint8_t MAX_SENSORS = 3;
    Sensor* sensors[MAX_SENSORS] = {};
    uint8_t sensorCount = 0;
public:
    void addSensor(Sensor* sensor);
    void initAll();
    void readAll();
    void updateAll();
    Sensor* getSensor(size_t index);
    size_t count() const;
};

#endif // SENSORS_H