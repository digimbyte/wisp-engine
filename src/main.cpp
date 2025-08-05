// Forward declarations
void bootloaderSetup();
void bootloaderLoop();

// ESP-IDF entry point
extern "C" void app_main() {
    bootloaderSetup();
    
    // Main loop
    while (true) {
        bootloaderLoop();
    }
}

// Arduino-style compatibility functions (if needed)
void setup() {
    bootloaderSetup();
}

void loop() {
    bootloaderLoop();
}
