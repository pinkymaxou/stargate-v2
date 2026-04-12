#pragma once

#include <cstdint>
#include "HW/SGHW_HAL.hpp"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/mcpwm_prelude.h"

class PinkySGHW : public SGHW_HAL
{

    #define STEPEND_BIT    0x01
    struct Stepper
    {
        esp_timer_handle_t signal_timer_handle;
        TaskHandle_t task_control_handle;

        int32_t period = 0;
        // Counter
        bool is_ccw;
        int32_t count = 0;
        int32_t target = 0;
    };

    struct ServoControl
    {
        mcpwm_timer_handle_t timer;
        mcpwm_oper_handle_t oper;
        mcpwm_cmpr_handle_t comparator;
        mcpwm_gen_handle_t generator;
    };

    public:
    PinkySGHW();

    void init() override;

    void setChevronLight(EChevron chevron, bool state) override;

    // Ramp light
    void setRampLight(double perc) override;

    void powerUpStepper() override;
    void stepStepperCW() override;
    void stepStepperCCW() override;
    void powerDownStepper() override;

    void powerUpServo() override;
    void setServo(double position) override;
    void powerDownServo() override;

    // Wormhole related
    int32_t getWHPixelCount() override;
    void setWHPixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) override;
    void clearAllWHPixels() override;
    bool refreshWHPixels() override;

    void setSanityLED(bool state);

    bool getIsHomeSensorActive() override;

    void sendMp3PlayerCMD(const char* cmd) override;

    // Stepper
    bool spinUntil(ESpinDirection spin_direction, ETransition transition, uint32_t timeout_ms, int32_t* ref_tick_count, const volatile bool* cancel_flag = nullptr) override;
    bool moveStepperTo(int32_t ticks, uint32_t timeout_ms) override;

    private:
    bool lockMutex() { return (pdTRUE == xSemaphoreTake( m_mutex_handle, ( TickType_t ) pdMS_TO_TICKS(100) )); }
    void unlockMutex() { xSemaphoreGive( m_mutex_handle ); }

    static void tmrSignalCallback(void* arg);

    private:
    Stepper m_stepper;
    ServoControl m_servo;

    led_strip_handle_t m_led_strip;

    double m_last_servo_position;

    // Mutex
    StaticSemaphore_t m_mutex_buffer; // Define the buffer for the mutex's data structure
    SemaphoreHandle_t m_mutex_handle; // Declare a handle for the mutex
};