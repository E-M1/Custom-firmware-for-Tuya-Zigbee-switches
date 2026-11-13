# Supported devices

> [!IMPORTANT]  
> Tuya devices share model names.  
> **Identify your device by Zigbee Manufacturer.** *Do not install if it does not match!*  

To support a new device, read [contribute/porting.md](/docs/contribute/porting.md) and ask for help.  
**Also read:** [recommended.md](./recommended.md) & [not_recommended.md](./not_recommended.md)  

### Legend

| Symbol | Meaning  |                    |                      |                    |                |           |
| :----: | ---------| ------------------ | -------------------- | ------------------ | -------------- | ----------|
|   🚧   | Status   | 🟩 Fully supported | 🟨 Mostly supported  | 🟧 In progress     | 🟥 Unsupported |           |
|   📦   | Build    | ✔️ Available       | ❌ Unavailable       |                    |                |           |
|   💡   | Category | 🇲 Module          | 🇸 Switch            | 🇴  Outlet         | 🇷 Remote      | 🇧  Board |
|   ⚡   | Power    | 🔌 Mains           | 🔋 Battery           | 🔱 USB             |                |           |
|   📲   | Install  | 🛜 Wireless        | ➿ By wire           | ❓ Some by wire    |                |           |
|   🏭   | MCU      | `TL` Telink        | `SL` Silicon Labs    |                    |                |           |
|   🅰    | Variant  | 🅰                  | 🅱                    | 🅲                  | 🅳              | 🅴         |

> [!NOTE]  
> Z2M pages are sometimes generic.  
> ***Look up devices in the linked threads and store listings!***

<!-------------------------------------------------------------------
  This page (`supported.md`) is generated. 
  
  Do not edit it directly! Instead, edit:
  - `device_db.yaml`             - add or edit devices
  - `supported_devices.md.jinja` - update the template
  - `make_supported_devices.py`  - update generation script

  Generate with: `make update_supported_devices`
-------------------------------------------------------------------->

### Device list

| 🚧 | 📦 | 💡 | ⚡ | 📲 | 🏭 | Zb&nbsp;Manufacturer <br> Zb&nbsp;Model | Name <br> Z2M&nbsp;page&nbsp;🔗 | Store | Threads | Status |
| -- | -- | -- | -- | -- | -- | :-------------------------------------- | :------------------------------ | ----: | ------: | :----- |
| 🟩 | ✔️ | 🇲 | 🔌 | 🛜 | **TL** | `_TZ3000_46t1rvdu` <br> `TS0001` | [Aubess WHD02](https://www.zigbee2mqtt.io/devices/WHD02.html) |   | [`#018`](https://github.com/romasku/tuya-zigbee-switch/issues/18) | Supported | 
| 🟩 | ✔️ | 🇲 | 🔌 | 🛜 | **TL** | `_TZ3000_lmlsduws` <br> `TS0002` | [Aubess TMZ02](https://www.zigbee2mqtt.io/devices/TMZ02.html) | [`AlEx`](https://www.aliexpress.com/item/1005005748264739.html) | [`#153`](https://github.com/romasku/tuya-zigbee-switch/pull/153) | Supported | 
| 🟩 | ✔️ | 🇲 | 🔌 | 🛜 | **TL** | `_TZ3000_lvhy15ix` <br> `TS0003` | [Aubess 3-gang](https://www.zigbee2mqtt.io/devices/TS0003_switch_module_2.html) | [`amzn`](https://www.amazon.co.uk/dp/B0DKC2CRFJ) | [`#151`](https://github.com/romasku/tuya-zigbee-switch/pull/151) | Supported | 
| 🟧 | ✔️ | 🇲 | 🔌 | ➿ | **TL** | `_TZ3218_7fiyo3kv` <br> `TS000F` | [Mumubiz TYZGTH1CH-D1RF](https://www.zigbee2mqtt.io/devices/TYZGTH1CH-D1RF.html) |   | [`#23974`](https://github.com/Koenkk/zigbee2mqtt/issues/23974) | Measured relay GPIO D2, pairing button GPIO C3, and DS18B20 temperature probe on D7. | 

Data from [`device_db.yaml`](/device_db.yaml)
