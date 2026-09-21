#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Bounce2.h>

#include "sensors.h"
#include "screens.h"

#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9
#define PIN_ENC_CLK 5
#define PIN_ENC_DT  7
#define PIN_ENC_SW  6
#define PIN_DUST_LED 10
#define PIN_DUST_OUT 0
#define PIN_MQ135_OUT 2
#define PIN_FAN_PWM 3

const uint32_t FAN_PWM_FREQ = 25000;
const uint8_t FAN_PWM_RES = 8;
const uint8_t FAN_PWM_CHANNEL = 0;
int fanSpeed = 128;

SystemState currentState = STATE_HOME_AUTO;
uint32_t lastInteractionTime = 0;
uint32_t lastScreenCycleTime = 0;
uint32_t lastSensorReadTime = 0;
uint32_t cycleInterval = 5000;
uint32_t timeoutInterval = 15000;
uint32_t sensorReadInterval = 2000;
uint32_t sleepInterval = 300000;

uint8_t currentScreenIndex = 0;
uint8_t menuIndex = 0;
IntervalSetting intervalSetting = INTERVAL_CYCLE;
uint32_t intervalEditValue = 0;
bool autoCycleEnabled = true;
bool useFahrenheit = false;
uint8_t statusSensorIndex = 0;
MenuSystem menuSystem;
uint32_t buttonPressStarted = 0;
bool longPressHandled = false;
const uint32_t LONG_PRESS_INTERVAL = 1000;

volatile int encoderCount = 0;

void IRAM_ATTR encoderISR() {
    static uint8_t old_AB = 3; 
    static int8_t encval = 0;
    static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

    old_AB <<= 2; 
    old_AB |= ((digitalRead(PIN_ENC_DT) << 1) | digitalRead(PIN_ENC_CLK));
    encval += enc_states[(old_AB & 0x0f)];

    if (encval > 3) {
        encoderCount++;
        encval = 0;
    } else if (encval < -3) {
        encoderCount--;
        encval = 0;
    }
}

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);
Bounce encoderButton = Bounce();

SensorManager sensorManager;
UIManager uiManager(u8g2);

AHT30Sensor ahtSensor;
MQ135Sensor mq135Sensor(PIN_MQ135_OUT);
GP2Y1010AU0FSensor dustSensor(PIN_DUST_LED, PIN_DUST_OUT);
TemperatureHumidityScreen temperatureScreen;
MQ135Screen mq135Screen;
DustScreen dustScreen;
SummaryScreen summaryScreen;

void resetIdleTimer() {
    lastInteractionTime = millis();
    if (currentState == STATE_HOME_AUTO) currentState = STATE_HOME_MANUAL;
}

// Return to the menu after completing an editor action.
void closeEditor() {
    currentState = STATE_MENU;
    lastInteractionTime = millis();
}

void beginIntervalEdit(IntervalSetting setting) {
    intervalSetting = setting;
    intervalEditValue = setting == INTERVAL_CYCLE ? cycleInterval :
        (setting == INTERVAL_TIMEOUT ? timeoutInterval :
         (setting == INTERVAL_SENSOR_READ ? sensorReadInterval : sleepInterval));
    currentState = STATE_INTERVAL;
}

void adjustInterval(int steps) {
    const uint32_t step = intervalSetting == INTERVAL_TIMEOUT || intervalSetting == INTERVAL_SLEEP ? 5000 : 1000;
    const uint32_t minimum = intervalSetting == INTERVAL_TIMEOUT ? 5000 : 1000;
    const uint32_t maximum = intervalSetting == INTERVAL_CYCLE ? 60000 :
        (intervalSetting == INTERVAL_TIMEOUT ? 120000 :
         (intervalSetting == INTERVAL_SENSOR_READ ? 30000 : 3600000));
    int32_t adjusted = static_cast<int32_t>(intervalEditValue) + steps * static_cast<int32_t>(step);
    intervalEditValue = constrain(adjusted, static_cast<int32_t>(minimum), static_cast<int32_t>(maximum));
}

void saveIntervalEdit() {
    if (intervalSetting == INTERVAL_CYCLE) cycleInterval = intervalEditValue;
    else if (intervalSetting == INTERVAL_TIMEOUT) timeoutInterval = intervalEditValue;
    else if (intervalSetting == INTERVAL_SENSOR_READ) sensorReadInterval = intervalEditValue;
    else sleepInterval = intervalEditValue;
    closeEditor();
}

void enterSleep() {
    currentState = STATE_SLEEP;
    u8g2.setPowerSave(1);
}

void wakeFromSleep() {
    u8g2.setPowerSave(0);
    lastInteractionTime = millis();
    currentState = autoCycleEnabled ? STATE_HOME_AUTO : STATE_HOME_MANUAL;
    lastScreenCycleTime = lastInteractionTime;
}

uint8_t currentMenuItemCount() {
    return menuSystem.itemCount();
}

bool isMenuState() {
    return currentState == STATE_MENU || currentState == STATE_FAN_SPEED ||
        currentState == STATE_INTERVAL || currentState == STATE_STATUS ||
        currentState == STATE_DEBUG;
}

void exitMenu() {
    currentState = autoCycleEnabled ? STATE_HOME_AUTO : STATE_HOME_MANUAL;
    menuSystem.reset();
    lastInteractionTime = millis();
}

void setup() {
    // Initialize display, fan PWM, encoder input, and sensors once.
    u8g2.begin();
    u8g2.setFontMode(1);
    menuSystem.reset();

    Serial.begin(115200);
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RES);
    ledcAttachPin(PIN_FAN_PWM, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, fanSpeed);

    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);

    encoderButton.attach(PIN_ENC_SW, INPUT_PULLUP);
    encoderButton.interval(25);

    sensorManager.addSensor(&ahtSensor);
    sensorManager.addSensor(&mq135Sensor);
    sensorManager.addSensor(&dustSensor);
    sensorManager.initAll();

    uiManager.addHomeScreen(&temperatureScreen);
    uiManager.addHomeScreen(&mq135Screen);
    uiManager.addHomeScreen(&dustScreen);
    uiManager.addHomeScreen(&summaryScreen);
}

void loop() {
    unsigned long currentMillis = millis();
    encoderButton.update();

    // Copy the ISR counter atomically before processing user input.
    int stepsMoved;
    noInterrupts();
    stepsMoved = encoderCount;
    encoderCount = 0;
    interrupts();

    // Sensor polling is independent from display navigation.
    sensorManager.updateAll();
    // Avoid I2C activity while navigating so sensor transactions cannot disturb UI timing.
    bool userInterfaceActive = currentState == STATE_MENU ||
        currentState == STATE_FAN_SPEED || currentState == STATE_INTERVAL ||
        currentState == STATE_STATUS;
    if (!userInterfaceActive && currentMillis - lastSensorReadTime >= sensorReadInterval) {
        sensorManager.readAll();
        lastSensorReadTime = currentMillis;
    }

    // Long-press handling uses elapsed time instead of blocking the loop.
    if (encoderButton.read() == LOW) {
        if (buttonPressStarted == 0) buttonPressStarted = currentMillis;
        if (!longPressHandled && currentMillis - buttonPressStarted >= LONG_PRESS_INTERVAL && isMenuState()) {
            exitMenu();
            longPressHandled = true;
        }
    } else {
        buttonPressStarted = 0;
        longPressHandled = false;
    }

    // The encoder either navigates, selects a screen, or edits a value.
    if (stepsMoved != 0) {
        resetIdleTimer();
        int direction = (stepsMoved > 0) ? 1 : -1;
        uint8_t screenCount = uiManager.getHomeScreenCount();

        if (currentState == STATE_SLEEP) {
            wakeFromSleep();
        } else if (currentState == STATE_HOME_AUTO || currentState == STATE_HOME_MANUAL) {
            if (screenCount > 0) {
                if (direction > 0) currentScreenIndex = (currentScreenIndex + 1) % screenCount;
                else currentScreenIndex = (currentScreenIndex == 0) ? screenCount - 1 : currentScreenIndex - 1;
            }
        } 
        else if (currentState == STATE_MENU) menuSystem.move(direction);
        else if (currentState == STATE_FAN_SPEED) {
            fanSpeed += stepsMoved * 10;
            if (fanSpeed > 255) fanSpeed = 255;
            if (fanSpeed < 0) fanSpeed = 0;
            ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
        } else if (currentState == STATE_INTERVAL) {
            adjustInterval(stepsMoved);
        }
    }

    // A press opens a menu or commits the current editor value.
    if (encoderButton.fell() && !longPressHandled) {
        resetIdleTimer();
        if (currentState == STATE_SLEEP) {
            wakeFromSleep();
        } else if (currentState == STATE_HOME_AUTO || currentState == STATE_HOME_MANUAL) {
            currentState = STATE_MENU;
            menuSystem.reset();
        } else if (currentState == STATE_MENU) {
            MenuId itemId = menuSystem.selectedId();
            if (itemId == MENU_OPTIONS || itemId == MENU_INTERVALS ||
                itemId == MENU_SCREENS || itemId == MENU_SENSORS) {
                menuSystem.enterSelected();
            } else if (itemId == MENU_PWM) currentState = STATE_FAN_SPEED;
            else if (itemId == MENU_UNIT) {
                useFahrenheit = !useFahrenheit;
                ahtSensor.setFahrenheit(useFahrenheit);
            } else if (itemId == MENU_AUTO_CYCLE) {
                autoCycleEnabled = !autoCycleEnabled;
                currentState = autoCycleEnabled ? STATE_HOME_AUTO : STATE_HOME_MANUAL;
            } else if (itemId >= MENU_SCREEN_TEMP && itemId <= MENU_SCREEN_SUMMARY) {
                currentScreenIndex = itemId - MENU_SCREEN_TEMP;
                currentState = STATE_HOME_MANUAL;
            } else if (itemId >= MENU_CYCLE && itemId <= MENU_SLEEP) {
                beginIntervalEdit(static_cast<IntervalSetting>(itemId - MENU_CYCLE));
            } else if (itemId >= MENU_SENSOR_AHT30 && itemId <= MENU_SENSOR_GP2Y) {
                statusSensorIndex = itemId - MENU_SENSOR_AHT30;
                currentState = STATE_STATUS;
            } else if (itemId == MENU_DEBUG) {
                currentState = STATE_DEBUG;
            } else if (itemId == MENU_BACK) {
                if (menuSystem.atRoot()) exitMenu();
                else menuSystem.back();
            }
        } else if (currentState == STATE_FAN_SPEED) {
            closeEditor();
        } else if (currentState == STATE_INTERVAL) {
            saveIntervalEdit();
        } else if (currentState == STATE_STATUS) {
            currentState = STATE_MENU;
        } else if (currentState == STATE_DEBUG) {
            currentState = STATE_MENU;
        }
    }

    // Only home and menu states use inactivity and auto-cycle timers.
    if (currentState == STATE_HOME_AUTO) {
        if (currentMillis - lastScreenCycleTime >= cycleInterval) {
            uint8_t screenCount = uiManager.getHomeScreenCount();
            if (screenCount > 0) currentScreenIndex = (currentScreenIndex + 1) % screenCount;
            lastScreenCycleTime = currentMillis;
        }
    } 
    else if (currentState == STATE_HOME_MANUAL) {
        if (currentMillis - lastInteractionTime >= timeoutInterval) {
            currentState = autoCycleEnabled ? STATE_HOME_AUTO : STATE_HOME_MANUAL;
            lastScreenCycleTime = currentMillis;
        }
    }

    // Evaluate the final state so opening a menu cannot sleep in this same loop.
    if (!isMenuState() && currentState != STATE_SLEEP &&
        currentMillis - lastInteractionTime >= sleepInterval) {
        enterSleep();
    }

    uint32_t cycleElapsed = currentMillis - lastScreenCycleTime;
    uint32_t cycleRemaining = cycleElapsed >= cycleInterval ? 0 : cycleInterval - cycleElapsed;
    DisplaySettings displaySettings = {
        autoCycleEnabled, cycleInterval, timeoutInterval, sensorReadInterval, sleepInterval,
        static_cast<uint8_t>((cycleRemaining + 999) / 1000),
        autoCycleEnabled && currentState == STATE_HOME_AUTO &&
        cycleRemaining > 0 && cycleRemaining <= 5000,
        autoCycleEnabled && currentState == STATE_HOME_AUTO &&
        (cycleRemaining > 5000 ? ((currentMillis / 1000) % 2 == 0) : true),
        useFahrenheit
    };
    uiManager.render(currentState, currentScreenIndex, menuSystem, fanSpeed,
                     intervalSetting, intervalEditValue, statusSensorIndex, displaySettings, sensorManager);
}
