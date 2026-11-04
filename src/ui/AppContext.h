#pragma once

class DeviceManager;
class ConfigStore;
class ToastHost;

/// Shared services handed to the pages.
struct AppContext {
    DeviceManager *devices = nullptr;
    ConfigStore *store = nullptr;
    ToastHost *toasts = nullptr;
    bool demo = false;
};
