#ifndef SCREENS_H
#define SCREENS_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "menu.h"
#include "sensors.h"

enum SystemState {
    STATE_HOME_AUTO,
    STATE_HOME_MANUAL,
    STATE_MENU,
    STATE_FAN_SPEED,
    STATE_INTERVAL,
    STATE_SLEEP,
    STATE_STATUS,
    STATE_DEBUG
};

enum IntervalSetting {
    INTERVAL_CYCLE,
    INTERVAL_TIMEOUT,
    INTERVAL_SENSOR_READ,
    INTERVAL_SLEEP
};

struct DisplaySettings {
    bool autoCycleEnabled;
    uint32_t cycleInterval;
    uint32_t timeoutInterval;
    uint32_t sensorReadInterval;
    uint32_t sleepInterval;
    uint8_t cycleSecondsRemaining;
    bool showCycleCountdown;
    bool cycleIndicatorVisible;
    bool useFahrenheit;
};

class HomeScreen {
public:
    virtual void draw(U8G2 &u8g2, SensorManager &sensors) = 0;
    virtual ~HomeScreen() {}
};

class TemperatureHumidityScreen : public HomeScreen {
public: void draw(U8G2 &u8g2, SensorManager &sensors) override;
};

class MQ135Screen : public HomeScreen {
public: void draw(U8G2 &u8g2, SensorManager &sensors) override;
};

class DustScreen : public HomeScreen {
public: void draw(U8G2 &u8g2, SensorManager &sensors) override;
};

class SummaryScreen : public HomeScreen {
public:
    void draw(U8G2 &u8g2, SensorManager &sensors) override;
};

class UIManager {
private:
    U8G2 &u8g2;
    HomeScreen* homeScreens[4] = {};
    uint8_t homeScreenCount = 0;

    void drawMenuScreen(const MenuSystem& menu, const DisplaySettings& settings);
    void drawFanSpeedScreen(int fanSpeed);
    void drawIntervalScreen(IntervalSetting setting, uint32_t value);
    void drawStatusScreen(SensorManager& sensors, uint8_t sensorIndex);
    void drawDebugScreen(SensorManager& sensors);
    void drawMenuIcon(uint8_t icon, uint8_t x, uint8_t y);
    void drawCycleCountdown(const DisplaySettings& settings);

public:
    explicit UIManager(U8G2 &display);
    ~UIManager();
    void addHomeScreen(HomeScreen* screen);
    uint8_t getHomeScreenCount() const;
    void render(SystemState state, uint8_t currentScreenIdx, const MenuSystem& menu,
                int fanSpeed, IntervalSetting intervalSetting, uint32_t intervalValue,
                uint8_t statusSensorIndex,
                const DisplaySettings& settings, SensorManager &sensorMgr);
};

#endif // SCREENS_H