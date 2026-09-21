#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

// Menu IDs are also indexes into the fixed node table.
enum MenuId : uint8_t {
    MENU_ROOT, MENU_BACK, MENU_OPTIONS, MENU_INTERVALS, MENU_SCREENS, MENU_SENSORS,
    MENU_PWM, MENU_UNIT, MENU_AUTO_CYCLE,
    MENU_CYCLE, MENU_TIMEOUT, MENU_READ, MENU_SLEEP,
    MENU_SCREEN_TEMP, MENU_SCREEN_MQ135, MENU_SCREEN_DUST, MENU_SCREEN_SUMMARY,
    MENU_SENSOR_AHT30, MENU_SENSOR_MQ135, MENU_SENSOR_GP2Y, MENU_DEBUG
};

class Menu;

struct MenuItem {
    MenuId id;
    const char* label;
    uint8_t icon;
    const Menu* child;
};

// A menu is a static object with a fixed child list. It owns no heap memory.
class Menu {
private:
    const char* title;
    const MenuItem* children;
    uint8_t childCount;
public:
    Menu(const char* name, const MenuItem* items, uint8_t count)
        : title(name), children(items), childCount(count) {}
    const char* name() const { return title; }
    const MenuItem* items() const { return children; }
    uint8_t size() const { return childCount; }
};

extern const Menu rootMenu;
extern const uint8_t MENU_COUNT;

class MenuSystem {
private:
    static const uint8_t MAX_DEPTH = 3;
    const Menu* stack[MAX_DEPTH] = {};
    uint8_t selections[MAX_DEPTH] = {};
    uint8_t depth = 0;
public:
    void reset();
    void move(int8_t amount);
    bool enterSelected();
    void back();
    bool atRoot() const { return depth <= 1; }
    uint8_t itemCount() const;
    uint8_t selection() const;
    const MenuItem* selected() const;
    MenuId selectedId() const;
    const Menu* current() const;
};

#endif // MENU_H
