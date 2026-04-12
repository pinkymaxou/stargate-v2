#include "GateControl.hpp"
#include "../Wormhole/Wormhole.hpp"
#include "../FWConfig.hpp"
#include "misc-formula.h"
#include "SGURing.hpp"
#include "SGUComm.hpp"
#include "../Ring/RingBLEClient.hpp"
#include "../Audio/SoundFX.hpp"

#define TAG "GateControl"

GateControl::GateControl() :
    m_is_cancel_action(false)
{
    m_semaphore_handle = xSemaphoreCreateMutexStatic(&m_semaphore_create_mutex);
}

void GateControl::init(SGHW_HAL* sghw_hal)
{
    m_sghw_hal = sghw_hal;
}

void GateControl::startTask()
{
    if (pdPASS != xTaskCreatePinnedToCore(taskRunning, "GateControl", FWCONFIG_GATECONTROL_STACKSIZE, (void*)this, FWCONFIG_GATECONTROL_PRIORITY_DEFAULT, &m_gate_control_handle, FWCONFIG_GATECONTROL_COREID))
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

void GateControl::queueAutoHome()
{
    const SCmd command = { .cmd = ECmd::AutoHome };
    priQueueAction(command);
}

void GateControl::queueAutoCalibrate()
{
    const SCmd cmd = { .cmd = ECmd::AutoCalibrate };
    priQueueAction(cmd);
}

void GateControl::queueDialAddress(GateAddress& ga)
{
    const SCmd cmd =
    {
        .cmd = ECmd::DialAddress,
        .dial_address = { .gate_address = ga, .wormhole_type = Wormhole::EType::NormalSGU }
    };
    priQueueAction(cmd);
}

void GateControl::queueManualWormhole(Wormhole::EType type)
{
    const SCmd cmd =
    {
        .cmd = ECmd::ManualWormhole,
        .manual_wormhole = { .wormhole_type = type }
    };
    priQueueAction(cmd);
}

void GateControl::abortAction()
{
    m_is_cancel_action = true;
}

void GateControl::priQueueAction(SCmd cmd)
{
    m_next_cmd = cmd;
}

void GateControl::taskRunning(void* arg)
{
    GateControl* gc = (GateControl*)arg;

    ESP_LOGI(TAG, "Gatecontrol task started and ready.");

    // Dialing
    while(true)
    {
        xSemaphoreTake(gc->m_semaphore_handle, portMAX_DELAY);
        gc->m_curr_cmd = gc->m_next_cmd;
        gc->m_is_cancel_action = false;
        gc->m_next_cmd.cmd = ECmd::Idle;   // Reset the command "queue"
        xSemaphoreGive(gc->m_semaphore_handle);

        if (ECmd::Idle == gc->m_curr_cmd.cmd)
        {
            // TODO: Will be replaced by a manual event.
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Reset the receiving ..
        xSemaphoreTake(gc->m_semaphore_handle, portMAX_DELAY);
        gc->m_errors[0] = '\0';
        gc->m_is_in_error = false;
        gc->m_last_error_code = SGResult::OK;
        xSemaphoreGive(gc->m_semaphore_handle);

        SGResult result = SGResult::OK;

        do {
            switch(gc->m_curr_cmd.cmd)
            {
                case ECmd::AutoCalibrate:
                {
                    gc->m_state_machine.processEvent(GateEvent::CmdCalibrate);
                    ESP_LOGI(TAG, "Autocalibrate in progress.");
                    result = gc->autoCalibrate();
                    if (SGResult::OK != result)
                    {
                        gc->m_state_machine.processEvent(GateEvent::OperationFailed);
                        break;
                    }
                    // Will move it at it's home position, it should go very fast.
                    ESP_LOGI(TAG, "Autocalibrate succeeded.");
                    gc->m_state_machine.processEvent(GateEvent::OperationComplete);

                    gc->m_state_machine.processEvent(GateEvent::CmdHome);
                    result = gc->autoHome();
                    if (SGResult::OK != result)
                    {
                        gc->m_state_machine.processEvent(GateEvent::OperationFailed);
                        break;
                    }
                    ESP_LOGI(TAG, "Auto-home succeeded.");
                    gc->m_state_machine.processEvent(GateEvent::OperationComplete);
                    break;
                }
                case ECmd::AutoHome:
                {
                    gc->m_state_machine.processEvent(GateEvent::CmdHome);
                    ESP_LOGI(TAG, "Auto-home started.");
                    result = gc->autoHome();
                    if (SGResult::OK != result)
                    {
                        gc->m_state_machine.processEvent(GateEvent::OperationFailed);
                        break;
                    }
                    ESP_LOGI(TAG, "Auto-home succeeded.");
                    gc->m_state_machine.processEvent(GateEvent::OperationComplete);
                    break;
                }
                case ECmd::KeyPress:
                {
                    // TODO: Keypress one by one
                    ESP_LOGI(TAG, "TODO: KeyPress");
                    break;
                }
                case ECmd::DialAddress:
                {
                    gc->m_state_machine.processEvent(GateEvent::CmdDial);
                    ESP_LOGI(TAG, "Dialing ....");
                    result = gc->dialAddress(gc->m_curr_cmd.dial_address);
                    if (SGResult::OK != result)
                    {
                        gc->m_state_machine.processEvent(GateEvent::OperationFailed);
                        break;
                    }
                    ESP_LOGI(TAG, "Dialing address succeeded.");
                    gc->m_state_machine.processEvent(GateEvent::OperationComplete);
                    break;
                }
                case ECmd::ManualWormhole:
                {
                    gc->m_state_machine.processEvent(GateEvent::CmdManualWormhole);
                    ESP_LOGI(TAG, "ManualWormhole, name: %s", Wormhole::getTypeText(gc->m_curr_cmd.manual_wormhole.wormhole_type));
                    Wormhole wm { gc->m_sghw_hal, gc->m_curr_cmd.manual_wormhole.wormhole_type };
                    wm.begin();
                    wm.openingAnimation();
                    while (!gc->m_is_cancel_action)
                    {
                        // Unlimited time, it violate laws of physics! (AKA the needs of the plot)
                        const SGResult wm_result = wm.runTicks();
                        if (SGResult::OK != wm_result)
                        {
                            result = wm_result;
                            break;
                        }
                    }
                    wm.closingAnimation();
                    wm.end();
                    if (gc->m_is_cancel_action)
                    {
                        gc->m_state_machine.processEvent(GateEvent::CmdAbort);
                    }
                    gc->m_state_machine.processEvent(GateEvent::OperationComplete);
                    break;
                }
                default:
                case ECmd::Idle:
                    break;
            }
        } while(false);

        if (SGResult::OK != result)
        {
            ESP_LOGE(TAG, "Error occurred: %s (code: %d)", getSGResultText(result), (int)result);

            // To be displayed into the web page.
            xSemaphoreTake(gc->m_semaphore_handle, portMAX_DELAY);
            strncpy(gc->m_errors, getSGResultText(result), ERROR_LEN);
            gc->m_last_error_code = result;
            gc->m_is_in_error = true;
            xSemaphoreGive(gc->m_semaphore_handle);
        }

        // Reset at the end, it's not really a queue
        gc->m_is_cancel_action = false;
        gc->m_curr_cmd.cmd = ECmd::Idle;
    }
}

SGResult GateControl::autoCalibrate()
{
    const uint32_t timeout = Settings::getI().getValueInt32(Settings::Entry::RingCalibTimeout);
    SGResult result = SGResult::Timeout;

    do {
        // We need two transitions from LOW to HIGH.
        // we give it 40s maximum to find the home.
        m_sghw_hal->powerUpStepper();
        releaseClamp();

        ESP_LOGI(TAG, "Finding home in progress");
        if (!m_sghw_hal->spinUntil(ESpinDirection::CCW, ETransition::Rising, timeout, nullptr, &m_is_cancel_action))
        {
            ESP_LOGE(TAG, "Calibration failed: %s finding first home position", m_is_cancel_action ? "cancelled" : "timeout");
            result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
            break;
        }

        ESP_LOGI(TAG, "Home has been found once");
        int32_t new_steps_per_rotation = 0;
        if (!m_sghw_hal->spinUntil(ESpinDirection::CCW, ETransition::Rising, timeout, &new_steps_per_rotation, &m_is_cancel_action))
        {
            ESP_LOGE(TAG, "Calibration failed: %s finding second home position", m_is_cancel_action ? "cancelled" : "timeout");
            result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
            break;
        }

        ESP_LOGI(TAG, "Home has been found a second time, step: %" PRId32, new_steps_per_rotation);

        // Find the gap.
        // Continue to move until it get out of the home range.
        int32_t gap = 0;

        if (!m_sghw_hal->spinUntil(ESpinDirection::CCW, ETransition::Failing, timeout, &gap, &m_is_cancel_action))
        {
            ESP_LOGE(TAG, "Calibration failed: %s measuring gap (failing edge)", m_is_cancel_action ? "cancelled" : "timeout");
            result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
            break;
        }
        if (!m_sghw_hal->spinUntil(ESpinDirection::CW, ETransition::Rising, timeout, &gap, &m_is_cancel_action))
        {
            ESP_LOGE(TAG, "Calibration failed: %s measuring gap (rising edge)", m_is_cancel_action ? "cancelled" : "timeout");
            result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
            break;
        }

        ESP_LOGI(TAG, "Ticks per rotation: %" PRId32 ", time per rotation, gap: % " PRId32, new_steps_per_rotation, gap);

        // Save the calibration result.
        Settings::getI().setValueInt32(Settings::Entry::StepsPerRotation, new_steps_per_rotation);
        Settings::getI().setValueInt32(Settings::Entry::RingHomeGapRange, gap);
        Settings::getI().commit();

        // Go into the other direction until it get out of the sensor
        lockClamp();
        m_sghw_hal->powerDownStepper();
        m_sghw_hal->powerDownServo();

        result = SGResult::OK;
    } while(false);

    if (SGResult::OK != result)
    {
        // Cleanup on error
        m_sghw_hal->powerDownStepper();
        lockClamp();
    }

    return result;
}

SGResult GateControl::autoHome()
{
    SGResult result = SGResult::Timeout;

    do {
        m_sghw_hal->powerUpStepper();
        releaseClamp();

        const int32_t new_steps_per_rotation = Settings::getI().getValueInt32(Settings::Entry::StepsPerRotation);
        const int32_t gap = Settings::getI().getValueInt32(Settings::Entry::RingHomeGapRange);
        if (0 == new_steps_per_rotation || 0 == gap)
        {
            ESP_LOGE(TAG, "Homing failed: calibration not done");
            result = SGResult::NotCalibrated;
            break;
        }

        const uint32_t timeout = Settings::getI().getValueInt32(Settings::Entry::RingCalibTimeout);

        // If the ring is already near the home sensor, we just need to move a little bit.
        if (m_sghw_hal->getIsHomeSensorActive())
        {
            ESP_LOGI(TAG, "Homing using the fast algorithm");

            if (!m_sghw_hal->spinUntil(ESpinDirection::CW, ETransition::Failing, timeout, nullptr, &m_is_cancel_action))
            {
                ESP_LOGE(TAG, "Homing failed: %s exiting home zone", m_is_cancel_action ? "cancelled" : "timeout");
                result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
                break;
            }
            if (!m_sghw_hal->spinUntil(ESpinDirection::CCW, ETransition::Rising, timeout, nullptr, &m_is_cancel_action))
            {
                ESP_LOGE(TAG, "Homing failed: %s re-entering home zone", m_is_cancel_action ? "cancelled" : "timeout");
                result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
                break;
            }
        }
        else
        {
            ESP_LOGI(TAG, "Homing using the slow algorithm");
            if (!m_sghw_hal->spinUntil(ESpinDirection::CCW, ETransition::Rising, timeout, nullptr, &m_is_cancel_action))
            {
                ESP_LOGE(TAG, "Homing failed: %s searching for home zone", m_is_cancel_action ? "cancelled" : "timeout");
                result = m_is_cancel_action ? SGResult::Cancelled : SGResult::Timeout;
                break;
            }
        }

        // Move by half the deadband offset.
        // this is the real 0 position
        const int32_t half_deadband = gap / 2;
        ESP_LOGI(TAG, "Moving a little bit to take care of the deadband, offset: %" PRId32, half_deadband);
        for(int i = 0; i < half_deadband; i++)
        {
            m_sghw_hal->stepStepperCCW();
            vTaskDelay(1);
        }

        m_current_position_ticks = 0;
        m_is_homing_done = true;

        lockClamp();
        // Go into the other direction until it get out of the sensor
        m_sghw_hal->powerDownStepper();

        result = SGResult::OK;
    } while(false);

    if (SGResult::OK != result)
    {
        lockClamp();
        // Go into the other direction until it get out of the sensor
        m_sghw_hal->powerDownStepper();
    }

    return result;
}

SGResult GateControl::dialAddress(const SDialArg& dial_arg)
{
    const int32_t new_steps_per_rotation = Settings::getI().getValueInt32(Settings::Entry::StepsPerRotation);
    SGResult result = SGResult::HardwareFailure;
    bool process_started = false;

    auto end_of_process = [&](bool is_error) -> void
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (is_error)
        {
            RingBLEClient::getI().sendGateAnimation(SGUCommNS::EChevronAnimation::Chevron_ErrorToOff);
            SoundFX::getI().playSound(SoundFX::FileID::SGU_7_lockfail, false);
        }
        else
        {
            RingBLEClient::getI().sendGateAnimation(SGUCommNS::EChevronAnimation::Chevron_FadeOut);
        }
        animRampLight(false);

        if (!is_error)
        {
            // If no error happened, just wait a little bit for the effect.
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        // Go back to home position
        ESP_LOGI(TAG, "Move near the home position");
        const int32_t move_ticks = MISCFA_CircleDiffd32(m_current_position_ticks, 0, new_steps_per_rotation);
        if (!m_sghw_hal->moveStepperTo(move_ticks, 30000))
        {
            return;
        }

        ESP_LOGI(TAG, "Confirm the home position");
        autoHome();

        m_sghw_hal->powerDownStepper();
        lockClamp();
    };

    do
    {
        Wormhole wm { m_sghw_hal, dial_arg.wormhole_type };
        SoundFX::getI().stopSound();
        m_sghw_hal->powerUpStepper();
        releaseClamp();
        process_started = true;

        if (!m_is_homing_done)
        {
            ESP_LOGE(TAG, "Dial failed: homing not done");
            result = SGResult::NotHomed;
            break;
        }

        animRampLight(true);
        vTaskDelay(pdMS_TO_TICKS(750));
        SoundFX::getI().playSound(SoundFX::FileID::SGU_1_beginroll, false);
        RingBLEClient::getI().sendGateAnimation(SGUCommNS::EChevronAnimation::Chevron_FadeIn);
        vTaskDelay(pdMS_TO_TICKS(750));

        bool dial_loop_ok = true;
        // const EChevron chevrons[] = { EChevron::Chevron1, EChevron::Chevron2, EChevron::Chevron3, EChevron::Chevron4, EChevron::Chevron5, EChevron::Chevron6, EChevron::Chevron7_Master, EChevron::Chevron8, EChevron::Chevron9 };
        for(int32_t i = 0; i < dial_arg.gate_address.getSymbolCount(); i++)
        {
            if (m_is_cancel_action)
            {
                ESP_LOGW(TAG, "Dial cancelled by user at symbol %d", (int)i);
                result = SGResult::Cancelled;
                dial_loop_ok = false;
                break;
            }

            const uint8_t symbol = dial_arg.gate_address.getSymbol(i);

            // Dial sequence ...
            const int32_t led_index = SGURingNS::SymbolToLedIndex(symbol);
            const double angle = (SGURingNS::LEDIndexToDeg(led_index));
            const int32_t symbol_to_ticks = -1*(angle/360)*new_steps_per_rotation;

            const int32_t move_ticks = MISCFA_CircleDiffd32(m_current_position_ticks, symbol_to_ticks, new_steps_per_rotation);

            ESP_LOGI(TAG, "led index: %" PRId32 ", angle: %.2f, symbol2Ticks: %" PRId32, led_index, angle, symbol_to_ticks);
            SoundFX::getI().playSound(SoundFX::FileID::SGU_6_lggroll, true);
            vTaskDelay(pdMS_TO_TICKS(250));
            if (!m_sghw_hal->moveStepperTo(move_ticks, 30000))
            {
                ESP_LOGE(TAG, "Dial failed: motor timeout at symbol %d", (int)i);
                result = SGResult::Timeout;
                dial_loop_ok = false;
                break;
            }
            SoundFX::getI().stopSound();
            SoundFX::getI().playSound(SoundFX::FileID::SGU_3_chevlck2, false);
            vTaskDelay(pdMS_TO_TICKS(1000));
            SoundFX::getI().playSound(SoundFX::FileID::SGU_2_chevlck, false);
            RingBLEClient::getI().sendLightUpSymbol(symbol);

            m_current_position_ticks = symbol_to_ticks;
            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        if (!dial_loop_ok || m_is_cancel_action)
        {
            if (m_is_cancel_action && SGResult::OK == result)
            {
                result = SGResult::Cancelled;
            }
            break;
        }

        // Play the wormhole idling animation
        SoundFX::getI().playSound(SoundFX::FileID::SGU_5_gateopen, false);
        wm.begin();
        wm.openingAnimation();

        const uint32_t start_ticks = xTaskGetTickCount();
        while (!m_is_cancel_action)
        {
            // 5 minutes
            if ((xTaskGetTickCount() - start_ticks) > pdMS_TO_TICKS(5*60*1000))
            {
                break;
            }
            const SGResult wm_result = wm.runTicks();
            if (SGResult::OK != wm_result)
            {
                result = wm_result;
                break;
            }
        }
        // Turn-off all symbols before killing the wormhole
        SoundFX::getI().playSound(SoundFX::FileID::SGU_4_gateclos, false);
        RingBLEClient::getI().sendGateAnimation(SGUCommNS::EChevronAnimation::Chevron_NoSymbols);
        vTaskDelay(pdMS_TO_TICKS(1000));
        wm.closingAnimation();
        wm.end();

        result = SGResult::OK;
    } while(false);

    if (process_started)
    {
        end_of_process(SGResult::OK != result);
    }

    return result;
}


void GateControl::animRampLight(bool is_active)
{
    const float pwm_on = (float)Settings::getI().getValueInt32(Settings::Entry::RampOnPercent) / 100.0f;
    const float inc = 0.005f;

    if (is_active)
    {
        for(float value = 0.0f; value <= 1.0f; value += inc)
        {
            // Log corrected
            m_sghw_hal->setRampLight(MISCFA_LinearizeLEDOutput(value)*pwm_on);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    else
    {
        for(float value = 1.0f; value >= 0.0f; value -= inc)
        {
            m_sghw_hal->setRampLight(MISCFA_LinearizeLEDOutput(value)*pwm_on);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void GateControl::getState(UIState& ui_state)
{
    xSemaphoreTake(m_semaphore_handle, portMAX_DELAY);
    ui_state.cmd = m_curr_cmd.cmd;
    ui_state.state = m_state_machine.getState();
    // Last error
    ui_state.has_last_error = m_is_in_error;
    ui_state.last_error_code = m_last_error_code;
    strcpy(ui_state.last_error, m_errors);

    strcpy(ui_state.status_text, getCmdText(m_curr_cmd.cmd));

    ui_state.is_cancel_requested = m_is_cancel_action;
    xSemaphoreGive(m_semaphore_handle);
}

void GateControl::releaseClamp()
{
    // Release the clamp
    m_sghw_hal->powerUpServo();
    m_sghw_hal->setServo(Settings::getI().getValueDouble(Settings::Entry::ClampReleasedPWM));
    vTaskDelay(pdMS_TO_TICKS(500));
}

void GateControl::lockClamp()
{
    m_sghw_hal->powerUpServo();
    m_sghw_hal->setServo(Settings::getI().getValueDouble(Settings::Entry::ClampLockedPWM));
    vTaskDelay(pdMS_TO_TICKS(500));
    m_sghw_hal->powerDownServo();
}