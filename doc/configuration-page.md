# Configuration Page

> **IMPLEMENTATION STATUS: PARTIALLY IMPLEMENTED**
>
> **Current implementation** (Settings tab in `/setup/index.html`):
> - ✅ View settings as table (key, description, type, min, default, max, value)
> - ✅ Fetch settings from `/api/settingsjson`
> - ✅ Inline editing of values via input fields
> - ✅ Save button (posts changes back to `/api/settingsjson`)
> - ❌ JSON file import/export via UI not implemented
>
> Users can use the API endpoint `/api/settingsjson` directly for full JSON export.

## Intended Functionality

Configurations could be imported or exported into JSON.

## JSON file viewer

The user should be able to view or directly edit the JSON file.
It's mostly done for backup or to change expert level configurations.

**Note**: This is the planned functionality, not the current implementation.

