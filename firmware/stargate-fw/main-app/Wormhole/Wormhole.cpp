#include <algorithm>
#include <cmath>
#include "Wormhole.hpp"
#include "../Settings.hpp"
#include "misc-formula.h"
#include "misc-macro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"

Wormhole::Wormhole(SGHW_HAL* hal, EType wormhole_type) :
    m_wormhole_type(wormhole_type),
    m_hal(hal)
{
    m_max_brightness = Settings::getI().getValueInt32(Settings::Entry::WormholeMaxLight);

    for(int i = 0; i < LEDEFFECT_COUNT; i++)
    {
        SLedEffect* led_effect = &m_led_effects[i];

        led_effect->is_up = false;
        led_effect->one = 0.0f;
    }
}

void Wormhole::begin()
{
    clearAll();
    m_is_run_initialized = false;
}

void Wormhole::openingAnimation()
{
    clearAll();
    illuminatring(ERing::Ring0, EDir::FadeIn);
    clearAll();
    illuminatring(ERing::Ring1, EDir::FadeIn);
    clearAll();
    illuminatring(ERing::Ring2, EDir::FadeIn);
    clearAll();
    illuminatring(ERing::Ring3, EDir::FadeIn);
    vTaskDelay(pdMS_TO_TICKS(150));
    illuminatring(ERing::Ring2, EDir::FadeIn);
    illuminatring(ERing::Ring1, EDir::FadeIn);
    illuminatring(ERing::Ring0, EDir::FadeIn);
}

SGResult Wormhole::runTicks()
{
    const float min_f = 0.30f;
    const float max_f = 1.00f;

    if (EType::Blackhole == m_wormhole_type)
    {
        static constexpr float TWO_PI = 6.2832f;

        m_bh_phase += 0.18f;
        if (m_bh_phase > TWO_PI)
            m_bh_phase -= TWO_PI;

        // Ring3 — event horizon: near-black, barely perceptible deep violet pulse
        {
            const float pulse = 0.5f + 0.5f * sinf(m_bh_phase * 0.2f);
            const uint8_t v = (uint8_t)(5.0f * pulse);
            for (int j = 0; j < RING3_COUNT; j++)
                m_hal->setWHPixel(m_ring3_one_based[j]-1, (uint8_t)(v * 0.3f), 0, v);
        }

        // Ring2 — photon ring: dominant feature, warm amber-orange, fast spin
        // The bright arc sweeps around simulating gravitational lensing
        {
            for (int j = 0; j < RING2_COUNT; j++)
            {
                const float t = (float)j / (float)RING2_COUNT;
                const float hot = 0.5f + 0.5f * sinf(t * TWO_PI - m_bh_phase * 3.5f);
                const uint8_t r = (uint8_t)(m_max_brightness * (0.55f + 0.45f * hot));
                const uint8_t g = (uint8_t)(r * (0.28f + 0.12f * hot));
                m_hal->setWHPixel(m_ring2_one_based[j]-1, r, g, 0);
            }
        }

        // Ring1 — accretion disk: orange-red, Doppler-shifted spin
        // Hot side brighter and more orange, cool side dimmer and redder
        {
            for (int j = 0; j < RING1_COUNT; j++)
            {
                const float t = (float)j / (float)RING1_COUNT;
                const float hot = 0.5f + 0.5f * sinf(t * TWO_PI - m_bh_phase * 1.5f);
                const uint8_t r = (uint8_t)(m_max_brightness * (0.35f + 0.30f * hot));
                const uint8_t g = (uint8_t)(r * 0.18f * hot);
                m_hal->setWHPixel(m_ring1_one_based[j]-1, r, g, 0);
            }
        }

        // Ring0 — outer nebula glow: very dim red, slow drift
        {
            for (int j = 0; j < RING0_COUNT; j++)
            {
                const float t = (float)j / (float)RING0_COUNT;
                const float hot = 0.5f + 0.5f * sinf(t * TWO_PI - m_bh_phase * 0.6f);
                const uint8_t r = (uint8_t)(m_max_brightness * 0.08f * (0.5f + 0.5f * hot));
                m_hal->setWHPixel(m_ring0_one_based[j]-1, r, (uint8_t)(r * 0.06f), 0);
            }
        }
    }
    else
    {
        if (!m_is_run_initialized)
        {
            for(int i = 0; i < m_hal->getWHPixelCount(); i++)
            {
                SLedEffect* led_effect = &m_led_effects[i];
                led_effect->one = min_f + (((esp_random() % 100) * 0.01f) * (max_f - min_f));
                led_effect->is_up = 0 != (esp_random() % 2);
            }
            m_is_run_initialized = true;
        }

        for(int i = 0; i < m_hal->getWHPixelCount(); i++)
        {
            SLedEffect* led_effect = &m_led_effects[i];

            const float inc = 0.0005f * (esp_random() % 100);

            led_effect->one += inc * (led_effect->is_up ? 1.0f : -1.0f);

            if (led_effect->one >= max_f)
            {
                led_effect->one = max_f;
                led_effect->is_up = false;
            }
            else if (led_effect->one <= min_f)
            {
                led_effect->one = min_f;
                led_effect->is_up = true;
            }

            float corr_value = MISCFA_LinearizeLEDOutput(led_effect->one);

            constexpr float ring_corr_values[(int)Wormhole::ERing::Count] = { 0.1f, 0.6f, 0.9f, 1.0f };
            corr_value *= ring_corr_values[(int)getRing(i)];

            const uint8_t pwm = (uint8_t)(corr_value * m_max_brightness);

            if (EType::NormalSG1 == m_wormhole_type)
            {
                m_hal->setWHPixel(i, MISCMACRO_MAX(pwm, 16), MISCMACRO_MAX(pwm, 16), MISCMACRO_MIN(16+pwm, m_max_brightness-16));
            }
            else if (EType::NormalSGU == m_wormhole_type)
            {
                m_hal->setWHPixel(i, pwm, pwm, pwm);
            }
        }
    }

    if (!m_hal->refreshWHPixels())
    {
        ESP_LOGW(TAG, "Error during refresh, may be caused by power instability");
        return SGResult::Wormhole_PowerInstability;
    }

    // 25 HZ
    vTaskDelay(pdMS_TO_TICKS(40));

    return SGResult::OK;
}

void Wormhole::closingAnimation()
{
    illuminatring(ERing::Ring3, EDir::FadeOut);
    illuminatring(ERing::Ring2, EDir::FadeOut);
    illuminatring(ERing::Ring1, EDir::FadeOut);
    illuminatring(ERing::Ring0, EDir::FadeOut);

    // Clear all pixels
    clearAll();
}

void Wormhole::end()
{
    clearAll();
}

Wormhole::ERing Wormhole::getRing(int zero_based_index)
{
    for(int j = 0; j < sizeof(m_ring0_one_based)/sizeof(m_ring0_one_based[0]); j++)
        if (m_ring0_one_based[j]-1 == zero_based_index)
            return Wormhole::ERing::Ring0;
    for(int j = 0; j < sizeof(m_ring1_one_based)/sizeof(m_ring1_one_based[0]); j++)
        if (m_ring1_one_based[j]-1 == zero_based_index)
            return Wormhole::ERing::Ring1;
    for(int j = 0; j < sizeof(m_ring2_one_based)/sizeof(m_ring2_one_based[0]); j++)
        if (m_ring2_one_based[j]-1 == zero_based_index)
            return Wormhole::ERing::Ring2;
    for(int j = 0; j < sizeof(m_ring3_one_based)/sizeof(m_ring3_one_based[0]); j++)
        if (m_ring3_one_based[j]-1 == zero_based_index)
            return Wormhole::ERing::Ring3;
    return Wormhole::ERing::Ring0;
}

void Wormhole::clearAll()
{
    m_hal->clearAllWHPixels();
    m_hal->refreshWHPixels();
}

void Wormhole::illuminatring(Wormhole::ERing ring, Wormhole::EDir dir)
{
    for(int32_t step = 0; step <= 100; step += 10)
    {
        const float brig = ((Wormhole::EDir::FadeOut == dir) ? (100-step) : step) * 0.01f;
        const SRingEntry* ring_entries = &m_ring_entries[(int)ring];
        for(int i = 0; i < ring_entries->ring_count; i++)
        {
            m_hal->setWHPixel(ring_entries->ring[i]-1, (uint8_t)(m_max_brightness * brig), (uint8_t)(m_max_brightness * brig), (uint8_t)(m_max_brightness * brig));
        }
        m_hal->refreshWHPixels();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
