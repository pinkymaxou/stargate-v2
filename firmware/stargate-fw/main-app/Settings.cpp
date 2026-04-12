#include "Settings.hpp"
#include <cstring>

void Settings::init()
{
    const NVSJSON_ESETRET ret = NVSJSON_Init(&m_setting_handle, &m_setting_config);
    assert(NVSJSON_ESETRET_OK == ret);
}

void Settings::load()
{
    NVSJSON_Load(&m_setting_handle);
}

void Settings::commit()
{
    NVSJSON_Save(&m_setting_handle);
}

int32_t Settings::getValueInt32(Settings::Entry entry)
{
    return NVSJSON_GetValueInt32(&m_setting_handle, (uint16_t)entry);
}

NVSJSON_ESETRET Settings::setValueInt32(Settings::Entry entry, bool is_dry_run, int32_t new_value)
{
    return NVSJSON_SetValueInt32(&m_setting_handle, (uint16_t)entry, is_dry_run, new_value);
}

NVSJSON_ESETRET Settings::setValueInt32(Settings::Entry entry, int32_t new_value)
{
    return NVSJSON_SetValueInt32(&m_setting_handle, (uint16_t)entry, false, new_value);
}

void Settings::getValueString(Settings::Entry entry, char* out_value, size_t* length)
{
    NVSJSON_GetValueString(&m_setting_handle, (uint16_t)entry, out_value, length);
}

NVSJSON_ESETRET Settings::setValueString(Settings::Entry entry, bool is_dry_run, const char* value)
{
    return NVSJSON_SetValueString(&m_setting_handle, (uint16_t)entry, is_dry_run, value);
}

double Settings::getValueDouble(Settings::Entry entry)
{
    return NVSJSON_GetValueDouble(&m_setting_handle, (uint16_t)entry);
}

NVSJSON_ESETRET Settings::setValueDouble(Settings::Entry entry, bool is_dry_run, double value)
{
    return NVSJSON_SetValueDouble(&m_setting_handle, (uint16_t)entry, is_dry_run, value);
}

NVSJSON_ESETRET Settings::setValueDouble(Settings::Entry entry, double value)
{
    return NVSJSON_SetValueDouble(&m_setting_handle, (uint16_t)entry, false, value);
}

char* Settings::exportJSON()
{
    return NVSJSON_ExportJSON(&m_setting_handle);
}

bool Settings::importJSON(const char* json)
{
    return NVSJSON_ImportJSON(&m_setting_handle, json);
}

bool Settings::validateWifiPassword(const NVSJSON_SSettingEntry* setting_entry, const char* value)
{
    const size_t n = strlen(value);
    return 8 < n || 0 == n;
}
