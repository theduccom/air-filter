#include "menu.h"

namespace {
const MenuItem optionsItems[] = {
    {MENU_BACK, "Back", 4, nullptr},
    {MENU_UNIT, "Units", 2, nullptr},
    {MENU_AUTO_CYCLE, "Auto cycle", 1, nullptr}
};
const MenuItem intervalItems[] = {
    {MENU_BACK, "Back", 4, nullptr},
    {MENU_CYCLE, "Screen cycle", 2, nullptr},
    {MENU_TIMEOUT, "Menu timeout", 2, nullptr},
    {MENU_READ, "Data read", 2, nullptr},
    {MENU_SLEEP, "Sleep timeout", 2, nullptr}
};
const MenuItem screenItems[] = {
    {MENU_BACK, "Back", 4, nullptr},
    {MENU_SCREEN_TEMP, "Temperature", 0, nullptr},
    {MENU_SCREEN_MQ135, "MQ135", 0, nullptr},
    {MENU_SCREEN_DUST, "Dust sensor", 0, nullptr},
    {MENU_SCREEN_SUMMARY, "Summary", 0, nullptr}
};
const MenuItem sensorItems[] = {
    {MENU_BACK, "Back", 4, nullptr},
    {MENU_SENSOR_AHT30, "AHT30", 0, nullptr},
    {MENU_SENSOR_MQ135, "MQ135", 0, nullptr},
    {MENU_SENSOR_GP2Y, "GP2Y1010AU0F", 0, nullptr}
};
const MenuItem rootItems[] = {
    {MENU_BACK, "Back", 4, nullptr},
    {MENU_OPTIONS, "Options", 1, nullptr},
    {MENU_INTERVALS, "Intervals", 2, nullptr},
    {MENU_SCREENS, "Screens", 0, nullptr},
    {MENU_SENSORS, "Sensors", 2, nullptr},
    {MENU_DEBUG, "Debug", 2, nullptr},
    {MENU_PWM, "Fan PWM", 3, nullptr}
};
}

// Static menu objects form the tree without runtime allocation.
const Menu optionsMenu("Options", optionsItems, 3);
const Menu intervalMenu("Intervals", intervalItems, 5);
const Menu screenMenu("Screens", screenItems, 5);
const Menu sensorMenu("Sensors", sensorItems, 4);
const Menu rootMenu("Settings", rootItems, 7);
const uint8_t MENU_COUNT = 7;

// Resolve submenu links without storing mutable pointers in the flash item tables.
static const Menu* childMenu(MenuId id) {
    if (id == MENU_OPTIONS) return &optionsMenu;
    if (id == MENU_INTERVALS) return &intervalMenu;
    if (id == MENU_SCREENS) return &screenMenu;
    if (id == MENU_SENSORS) return &sensorMenu;
    return nullptr;
}

void MenuSystem::reset() {
    stack[0] = &rootMenu;
    selections[0] = 0;
    depth = 1;
}

void MenuSystem::move(int8_t amount) {
    if (depth == 0 || stack[depth - 1]->size() == 0) return;
    int16_t next = selections[depth - 1] + amount;
    if (next < 0) next = stack[depth - 1]->size() - 1;
    if (next >= stack[depth - 1]->size()) next = 0;
    selections[depth - 1] = static_cast<uint8_t>(next);
}

bool MenuSystem::enterSelected() {
    const MenuItem* item = selected();
    const Menu* child = item ? childMenu(item->id) : nullptr;
    if (!child || depth >= MAX_DEPTH) return false;
    stack[depth] = child;
    selections[depth++] = 0;
    return true;
}

void MenuSystem::back() { if (depth > 1) --depth; }

uint8_t MenuSystem::itemCount() const {
    return depth == 0 ? 0 : stack[depth - 1]->size();
}

uint8_t MenuSystem::selection() const {
    return depth == 0 ? 0 : selections[depth - 1];
}

const MenuItem* MenuSystem::selected() const {
    if (depth == 0 || selections[depth - 1] >= itemCount()) return nullptr;
    return &stack[depth - 1]->items()[selections[depth - 1]];
}

MenuId MenuSystem::selectedId() const {
    const MenuItem* item = selected();
    return item ? item->id : MENU_BACK;
}

const Menu* MenuSystem::current() const {
    return depth == 0 ? nullptr : stack[depth - 1];
}
