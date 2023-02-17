#include "RF24.h"
#include "lcd_display.hpp"
#include <stdlib.h>
#include "pico/stdlib.h"

#define BUILT_IN_LED PICO_DEFAULT_LED_PIN

// nRF24L01 control pins
#define SPI_CE_PIN 14
#define SPI_CS_PIN 15

// 2x16 LCD control pins
#define PIN4 11
#define PIN5 10
#define PIN6 9
#define PIN7 8
#define RS_PIN 13
#define E_PIN 12
#define REFRESH_TIME_MS 200 // ms

// Buttons pins
#define START_BTN 4
#define STOP_BTN 3
#define TEST_BTN 2 // Used to test connection with finnish line module

namespace Stopwatch
{
    enum State : uint8_t
    {
        waiting_for_start = 0,
        measuring_time = 1,
        time_stopped = 2
    };

    void initPushButton(uint pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        return;
    }

}

int main()
{
    // Periferals setup
    RF24 radio(SPI_CE_PIN, SPI_CS_PIN);
    LCDdisplay display(PIN4, PIN5, PIN6, PIN7, RS_PIN, E_PIN, 16, 2);

    // Check if initialzation succeeded
    if (!radio.begin())
    {
        return 0;
    }

    // Set radio power
    radio.setPALevel(RF24_PA_LOW);
    int payload;

    // Radio module address
    uint8_t address[2][6] = {"1Node", "2Node"};
    radio.setChannel(124);
    radio.setPayloadSize(sizeof(payload));

    // Open receiving pipe with address "1Node"
    radio.openReadingPipe(1, address[0]);

    // Open writing pipe with address "2Node"
    radio.openWritingPipe(address[1]);

    // Initialize display
    display.init();
    display.clear();
    display.print("0.00 s");

    // Initialize serial port and gpio
    stdio_init_all();

    // Turn on built in led (Initialization succeeded)
    gpio_init(BUILT_IN_LED);
    gpio_set_dir(BUILT_IN_LED, GPIO_OUT);
    gpio_put(BUILT_IN_LED, true);

    // Init buttons
    Stopwatch::initPushButton(START_BTN);
    Stopwatch::initPushButton(STOP_BTN);
    Stopwatch::initPushButton(TEST_BTN);

    // Current state
    Stopwatch::State currentState = Stopwatch::State::waiting_for_start;

    // Timestamp
    absolute_time_t startTimestamp;
    uint32_t lastDisplayRefreshTimeMs;

    // Switch to TX mode
    radio.stopListening();

    // Program loop
    while (true)
    {
        while (currentState == Stopwatch::State::waiting_for_start)
        {
            if (!gpio_get(START_BTN))
            {
                startTimestamp = get_absolute_time();
                lastDisplayRefreshTimeMs = to_ms_since_boot(startTimestamp);
                display.clear();
                display.print("0.00 s");

                // Switch to RX mode
                radio.startListening();
                currentState = Stopwatch::State::measuring_time;
            }

            if (!gpio_get(TEST_BTN))
            {
                int payload = 0;
                display.goto_pos(0, 1);
                if (radio.write(&payload, sizeof(payload)))
                {
                    display.print("connected");
                }
                else
                {
                    display.print("disconnected");
                }
            }
        }

        while (currentState == Stopwatch::State::measuring_time)
        {
            if (radio.available() || !gpio_get(STOP_BTN))
            {
                // Get current timestamp and claculate time between start and stop
                auto stopTimestamp = get_absolute_time();
                double measuredTimeS = static_cast<double>(to_ms_since_boot(stopTimestamp) - to_ms_since_boot(startTimestamp)) / 1000;

                auto bytes = radio.getPayloadSize();
                radio.read(&payload, bytes);

                radio.stopListening();

                // Prepare message
                char messageBuffer[40];
                snprintf(messageBuffer, 40, "Time: %.2f s", measuredTimeS);
                display.clear();
                display.print(messageBuffer);

                // Change state
                currentState = Stopwatch::State::time_stopped;
            }
            else
            {
                auto currentTimestampMs = to_ms_since_boot(get_absolute_time());
                bool refresh = (currentTimestampMs - lastDisplayRefreshTimeMs) > REFRESH_TIME_MS;

                if (refresh)
                {
                    auto currentTimeS = static_cast<double>(currentTimestampMs - to_ms_since_boot(startTimestamp)) / 1000;
                    char messageBuffer[40];
                    snprintf(messageBuffer, 40, "%.2f s", currentTimeS);
                    display.clear();
                    display.print(messageBuffer);
                    lastDisplayRefreshTimeMs = currentTimestampMs;
                }
            }
        }

        while (currentState == Stopwatch::State::time_stopped)
        {
            if (!gpio_get(TEST_BTN))
            {
                display.clear();
                display.print("Time: 0.00 s");
                currentState = Stopwatch::State::waiting_for_start;
            }
        }
    }

    return 0;
}