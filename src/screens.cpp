#include "screens.h"

void TemperatureHumidityScreen::draw(U8G2 &u8g2, SensorManager &sensors) {
    // Keep labels short so values remain inside the 128-pixel viewport.
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, "Temp / Humidity");
    u8g2.drawHLine(0, 15, 128);

    AHT30Sensor* aht = static_cast<AHT30Sensor*>(sensors.getSensor(0));
    u8g2.setCursor(5, 35);
    u8g2.print("Temp: ");
    if (aht && aht->isAvailable()) u8g2.print(aht->getDisplayedTemperature(), 1);
    else u8g2.print("N/A");
    u8g2.print(aht ? aht->getTemperatureUnit() : " C");
    u8g2.setCursor(5, 55);
    u8g2.print("Hum:  ");
    if (aht && aht->isAvailable()) u8g2.print(aht->getHumidity(), 1);
    else u8g2.print("N/A");
    u8g2.print(" %");
}

void MQ135Screen::draw(U8G2 &u8g2, SensorManager &sensors) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, "MQ135 Air Quality");
    u8g2.drawHLine(0, 15, 128);

    MQ135Sensor* mq135 = static_cast<MQ135Sensor*>(sensors.getSensor(1));
    u8g2.setCursor(5, 35);
    u8g2.print("Analog: ");
    u8g2.print(mq135 ? mq135->getRawValue() : 0);
    u8g2.setCursor(5, 55);
    u8g2.print("Higher = more gas");
}

void DustScreen::draw(U8G2 &u8g2, SensorManager &sensors) {
    // Show the density and the AQI category from the PM2.5 table.
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, "GP2Y1010AU0F PM2.5");
    u8g2.drawHLine(0, 15, 128);

    GP2Y1010AU0FSensor* dust = static_cast<GP2Y1010AU0FSensor*>(sensors.getSensor(2));
    u8g2.setCursor(5, 30);
    u8g2.print("Density: ");
    u8g2.print(dust ? dust->getDensity() : 0, 0);
    u8g2.print(" ug/m3");
    u8g2.setCursor(5, 45);
    u8g2.print("AQI: ");
    u8g2.print(dust ? dust->getAqi() : 0);
    u8g2.setCursor(5, 60);
    u8g2.print(dust ? dust->getQuality() : "N/A");
}

void SummaryScreen::draw(U8G2 &u8g2, SensorManager &sensors) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, "Air Filter Summary");
    u8g2.drawHLine(0, 15, 128);

    AHT30Sensor* aht = static_cast<AHT30Sensor*>(sensors.getSensor(0));
    MQ135Sensor* mq135 = static_cast<MQ135Sensor*>(sensors.getSensor(1));
    GP2Y1010AU0FSensor* dust = static_cast<GP2Y1010AU0FSensor*>(sensors.getSensor(2));

    u8g2.setCursor(5, 27);
    u8g2.print("T:");
    if (aht && aht->isAvailable()) u8g2.print(aht->getDisplayedTemperature(), 1);
    else u8g2.print("N/A");
    u8g2.print(" H:");
    if (aht && aht->isAvailable()) u8g2.print(aht->getHumidity(), 1);
    else u8g2.print("N/A");
    u8g2.setCursor(5, 41);
    u8g2.print("MQ135: ");
    u8g2.print(mq135 ? mq135->getRawValue() : 0);
    u8g2.setCursor(5, 55);
    u8g2.print("PM2.5: ");
    u8g2.print(dust ? dust->getDensity() : 0, 0);
    u8g2.print(" AQI ");
    u8g2.print(dust ? dust->getAqi() : 0);
}

UIManager::UIManager(U8G2 &display) : u8g2(display) {}
UIManager::~UIManager() {}
void UIManager::addHomeScreen(HomeScreen* screen) {
    if (homeScreenCount < 4) homeScreens[homeScreenCount++] = screen;
}
uint8_t UIManager::getHomeScreenCount() const { return homeScreenCount; }

// Draw compact, informative icons without bitmap assets in flash.
void UIManager::drawMenuIcon(uint8_t icon, uint8_t x, uint8_t y) {
    switch (icon) {
        case 0: // display
            u8g2.drawFrame(x, y - 8, 10, 7);
            u8g2.drawHLine(x + 3, y, 4);
            break;
        case 1: // toggle
            u8g2.drawRFrame(x, y - 7, 12, 6, 3);
            u8g2.drawDisc(x + 3, y - 4, 2);
            break;
        case 2: // clock
            u8g2.drawCircle(x + 5, y - 4, 5);
            u8g2.drawLine(x + 5, y - 4, x + 5, y - 7);
            u8g2.drawLine(x + 5, y - 4, x + 8, y - 2);
            break;
        case 3: // fan
            u8g2.drawCircle(x + 5, y - 4, 2);
            u8g2.drawCircle(x + 5, y - 4, 5);
            u8g2.drawLine(x + 5, y - 4, x + 9, y - 7);
            u8g2.drawLine(x + 5, y - 4, x + 1, y - 1);
            break;
        default: // back
            u8g2.drawLine(x + 10, y - 4, x + 2, y - 4);
            u8g2.drawLine(x + 2, y - 4, x + 6, y - 8);
            u8g2.drawLine(x + 2, y - 4, x + 6, y);
            break;
    }
}

void UIManager::drawMenuScreen(const MenuSystem& menu, const DisplaySettings& settings) {
    const uint8_t visibleRows = 4;
    const uint8_t rowHeight = 11;
    uint8_t itemCount = menu.itemCount();
    if (itemCount == 0 || menu.current() == nullptr) return;
    uint8_t selected = menu.selection();
    if (selected >= itemCount) selected = 0;
    uint8_t firstRow = 0;
    if (itemCount > visibleRows) {
        if (selected >= visibleRows / 2) firstRow = selected - visibleRows / 2;
        if (firstRow > itemCount - visibleRows) firstRow = itemCount - visibleRows;
    }

    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "SETTINGS");
    u8g2.drawHLine(0, 15, 128);

    for (uint8_t row = 0; row < visibleRows; row++) {
        uint8_t itemIndex = firstRow + row;
        if (itemIndex >= itemCount) break;
        const MenuItem& item = menu.current()->items()[itemIndex];
        uint8_t yPos = 25 + row * rowHeight;
        if (itemIndex == selected) {
            u8g2.drawBox(0, yPos - 8, 121, 10);
            u8g2.setDrawColor(0);
        }
        drawMenuIcon(item.icon, 2, yPos);
        u8g2.setCursor(17, yPos);
        u8g2.print(item.label);
        if (item.id == MENU_AUTO_CYCLE) {
            u8g2.setCursor(98, yPos);
            u8g2.print(settings.autoCycleEnabled ? "ON" : "OFF");
        } else if (item.id == MENU_UNIT) {
            u8g2.setCursor(98, yPos);
            u8g2.print(settings.useFahrenheit ? "F" : "C");
        }
        u8g2.setDrawColor(1);
    }

    // The thumb shows the selected item among the complete menu.
    u8g2.drawFrame(124, 17, 4, 46);
    uint8_t thumbHeight = (46 * visibleRows) / itemCount;
    if (thumbHeight < 6) thumbHeight = 6;
    uint8_t thumbY = itemCount <= visibleRows ? 18 :
        18 + ((46 - thumbHeight) * firstRow) / (itemCount - visibleRows);
    u8g2.drawBox(125, thumbY, 2, thumbHeight);
}

void UIManager::drawFanSpeedScreen(int fanSpeed) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(25, 12, "Fan Speed");
    u8g2.drawHLine(0, 15, 128);

    int percent = map(fanSpeed, 0, 255, 0, 100);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(40, 40);
    u8g2.print(percent); u8g2.print(" %");

    u8g2.drawFrame(14, 50, 100, 10);
    u8g2.drawBox(14, 50, percent, 10);
}

void UIManager::drawIntervalScreen(IntervalSetting setting, uint32_t value) {
    const char* title = setting == INTERVAL_CYCLE ? "Screen cycle" :
        (setting == INTERVAL_TIMEOUT ? "Menu timeout" :
         (setting == INTERVAL_SENSOR_READ ? "Data read" : "Sleep timeout"));
    // Editors are intentionally simple: turn to change, press to save.
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, title);
    u8g2.drawHLine(0, 15, 128);
    u8g2.setCursor(27, 38);
    u8g2.print(value / 1000);
    u8g2.print(" seconds");
    u8g2.drawStr(20, 58, "Turn: change  Push: save");
}

void UIManager::drawStatusScreen(SensorManager& sensors, uint8_t sensorIndex) {
    AHT30Sensor* aht = static_cast<AHT30Sensor*>(sensors.getSensor(0));
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, sensorIndex == 0 ? "AHT30 status" :
        (sensorIndex == 1 ? "MQ135 status" : "Dust status"));
    u8g2.drawHLine(0, 15, 128);
    u8g2.setCursor(5, 30);
    if (sensorIndex == 0) {
        u8g2.print("Init: ");
        u8g2.print(aht && aht->isAvailable() ? "OK" : "FAIL");
    } else if (sensorIndex == 1) {
        u8g2.print("Analog input: READY");
    } else {
        u8g2.print("LED + analog: READY");
    }
}

void UIManager::drawCycleCountdown(const DisplaySettings& settings) {
    if (settings.showCycleCountdown) {
        // The number occupies the same small top-right slot as the clock icon.
        u8g2.setCursor(115, 10);
        u8g2.print(settings.cycleSecondsRemaining);
    } else if (settings.cycleIndicatorVisible) {
        u8g2.drawCircle(118, 6, 3);
        u8g2.drawLine(118, 6, 118, 4);
        u8g2.drawLine(118, 6, 120, 7);
    }
}

void UIManager::render(SystemState state, uint8_t currentScreenIdx, const MenuSystem& menu,
                       int fanSpeed, IntervalSetting intervalSetting, uint32_t intervalValue,
                       uint8_t statusSensorIndex, const DisplaySettings& settings, SensorManager &sensorMgr) {
    u8g2.firstPage();
    do {
        if (state == STATE_HOME_AUTO || state == STATE_HOME_MANUAL) {
            if (currentScreenIdx < homeScreenCount) {
                homeScreens[currentScreenIdx]->draw(u8g2, sensorMgr);
            }
            if (state == STATE_HOME_MANUAL) u8g2.drawBox(120, 58, 6, 6);
            else drawCycleCountdown(settings);
        } 
        else if (state == STATE_MENU) drawMenuScreen(menu, settings);
        else if (state == STATE_FAN_SPEED) drawFanSpeedScreen(fanSpeed);
        else if (state == STATE_INTERVAL) drawIntervalScreen(intervalSetting, intervalValue);
        else if (state == STATE_STATUS) drawStatusScreen(sensorMgr, statusSensorIndex);
        else if (state == STATE_DEBUG) drawDebugScreen(sensorMgr);
    } while (u8g2.nextPage());
}

void UIManager::drawDebugScreen(SensorManager& sensors) {
    AHT30Sensor* aht = static_cast<AHT30Sensor*>(sensors.getSensor(0));
    MQ135Sensor* mq135 = static_cast<MQ135Sensor*>(sensors.getSensor(1));
    GP2Y1010AU0FSensor* dust = static_cast<GP2Y1010AU0FSensor*>(sensors.getSensor(2));
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 12, "Debug raw values");
    u8g2.drawHLine(0, 15, 128);
    u8g2.setCursor(5, 28); u8g2.print("AHT T: "); u8g2.print(aht ? aht->getRawTemperature() : 0);
    u8g2.setCursor(5, 41); u8g2.print("AHT H: "); u8g2.print(aht ? aht->getRawHumidity() : 0);
    u8g2.setCursor(5, 54); u8g2.print("MQ: "); u8g2.print(mq135 ? mq135->getRawValue() : 0);
    u8g2.print(" D: "); u8g2.print(dust ? dust->getRawValue() : 0);
}