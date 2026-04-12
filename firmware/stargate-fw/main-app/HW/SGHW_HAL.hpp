#pragma once

#include <cstdint>
#include "../Gate/Chevron.hpp"

enum class MotorDirection
{
    Stop = 0,

    Forward,
    Backward
};

enum class ETransition
{
    Rising,
    Failing
};

enum class ESpinDirection
{
    CCW,
    CW
};

class SGHW_HAL
{
    public:
    SGHW_HAL() { }

    public:
    /*! @brief Initialize all pins and driver. Should be done fast after the MCU startup */
    virtual void init() { }

    /*! @brief Set chevron lightning status
        @param chevron     Chevron number
        @param state       OFF or ON*/
    virtual void setChevronLight(EChevron chevron, bool state) { }

    /*! @brief Ramp lightning output, based on a PWM.
        @param perc    PWM value between [0 and 1] */
    virtual void setRampLight(double perc) { };

    /*! @brief EChevron motor control, each chevron have one motor.
        @param chevron     Chevron number
        @param perc        PWM value between [0 and 1] */
    virtual void movChevronMotor(EChevron chevron, MotorDirection motor_dir) { }

    // Stepper.
    /*! @brief Active the power on the stepper driver and motor. */
    virtual void powerUpStepper() { }
    /*! @brief Move the stepper, basically if you stand in front of the gate the ring will spin clockwise.  */
    virtual void stepStepperCW() { }
    /*! @brief Move the stepper, basically if you stand in front of the gate the ring will spin counter-clockwise.  */
    virtual void stepStepperCCW() { }
    /*! @brief Power down the stepper driver. It helps to reduce motor heating when on IDLE. */
    virtual void powerDownStepper() { }

    // Servo motor
    /*! @brief Active the power on the servo motor. */
    virtual void powerUpServo() { }
    /*! @brief Change the servo position.
        @param position    New position between [0, 1] */
    virtual void setServo(double position) { }
    /*! @brief Power down the servo motor, it could be noisy while powered up.. */
    virtual void powerDownServo() { }

    // Wormhole related
    /*! @brief Get wormhole neopixel led strip count. */
    virtual int32_t getWHPixelCount() { return 0; }

    /*! @brief Set neopixel color on the led strip, by index.
        @param index LED index
        @param red    Red channel [0, 255]
        @param green  Green channel [0, 255]
        @param blue   Blue channel [0, 255] */
    virtual void setWHPixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) { }

    /*! @brief Clear all wormhole led strip pixels. */
    virtual void clearAllWHPixels() { }
    /*! @brief Refresh the wormhole led strip. */
    virtual bool refreshWHPixels() { return true; }

    /*! @brief Set sanity the LED status.
        @param state   false: light is off, true: light is on */
    virtual void setSanityLED(bool state) { }

    /*! @brief Is home sensor active ? Meaning the magnet near the home point. */
    virtual bool getIsHomeSensorActive() { return false; }

    virtual void sendMp3PlayerCMD(const char* cmd) { }

    // Stepper
    virtual bool spinUntil(ESpinDirection spin_direction, ETransition transition, uint32_t timeout_ms, int32_t* ref_tick_count, const volatile bool* cancel_flag = nullptr) { return false;};

    virtual bool moveStepperTo(int32_t ticks, uint32_t timeout_ms) { return false;};
};
