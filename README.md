# nrf52840-promicro-matter
Matter contact sensor example on Promicro Board
> This project ports the Nordic nRF Connect SDK Matter Contact Sensor sample to the nRF52840 ProMicro board. The goal is to run Matter over Thread and use USB DFU/UF2 for flashing firmware to avoid repeated operations on SWD pads.

## 0. Quick start
Release folder contain
```text
relase/
├── nice_nano_bootloader-0.11.0_s140_6.1.1.hex
├── uf2.hex
├── uf2.uf2
├── factory_data.txt
├── factory_data.json
├── factory_data.png
└── ...
```
* Flash adafruit bootloader to your promicro Board
* Double-tap reset to DFU mode
* Copy uf2.uf2 to NICENANO flash drive.
* Add matter device by homeassistant companion app, assume you adredy have working Thread network on your HA, and syn your Thread credentials to `HA companion apps`

## 1. Overview and Limitations

The original Nordic example targets the nRF52840 DK. ProMicro differs in hardware, particularly regarding external QSPI flash and pin wiring connected to the Adafruit bootloader reset line.

Key project adjustments:

* Modified Devicetree overlays for ProMicro. The project overrides both the main application overlay (`boards/nrf52840dk_nrf52840.overlay`) and the MCUboot child image overlay (`sysbuild/mcuboot/boards/nrf52840dk_nrf52840.overlay`) to maintain simple build commands while keeping partition layouts identical across both targets. These files are tailored for ProMicro and **no longer represent original DK configurations**.

* Disabled external flash not present on the ProMicro target configuration.

* Disabled DK library
   ```text
    CONFIG_DK_LIBRARY=n
    CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER=n
    CONFIG_NCS_SAMPLE_MATTER_LEDS=n
   ```
* Configured Matter application `slot1` as a 4 KiB placeholder partition in internal flash to satisfy build/configuration requirements (**does not support dual-slot Matter OTA**).

* Utilized boot chain: `Adafruit UF2 bootloader → MCUboot → Matter application`.

* Merged target HEX files post-build into a single UF2 binary for USB flashing.

USB UF2 is used as a manual firmware update method, not a Matter OTA replacement.

## 2. Hardware and Bootloader

### Hardware

* Board: nRF52840 ProMicro

* Base Application: Nordic Matter Contact Sensor Sample

* Communication Protocol: Thread

### Tested Adafruit UF2 Bootloader Details

Bootloader reported information:

* Version: `UF2 Bootloader 0.11.0`

* Model: `nice!nano`

* Board-ID: `nRF52840-nicenano`

* Date: `May 21 2026`

* SoftDevice: `S140 6.1.1`

## 3. Flash Layout

The nRF52840 provides 1 MiB internal flash (`0x00000`–`0xFFFFF`).

| Partition | Start Address | Size | End Address (Inclusive) | Purpose | 
| ----- | ----- | ----- | ----- | ----- | 
| Lower Area / MBR | `0x00000` | 4 KiB | `0x00FFF` | System low-level vector / MBR | 
| MCUboot | `0x01000` | 28 KiB | `0x07FFF` | Secondary Bootloader | 
| `image-0` / slot0 | `0x08000` | 896 KiB | `0xE7FFF` | Primary Application (Matter) | 
| `image-1` / slot1 | `0xE8000` | 4 KiB | `0xE8FFF` | Placeholder (Insufficient for OTA image) | 
| `factory-data` | `0xE9000` | 4 KiB | `0xE9FFF` | Matter Factory Data | 
| `storage` | `0xEA000` | 40 KiB | `0xF3FFF` | NVS, Thread credentials & persistent storage | 
| Adafruit Bootloader Region | `0xF4000` | 48 KiB | `0xFFFFF` | Primary USB Bootloader | 
* Important note: `Adafruit UF2 bootloader DFU writeable range is 0x1000 - 0xe9fff, everything out of this range will be ignored without alarm`.

## 4. Windows Environment Setup

### Prerequisites

1. Visual Studio Code with the **nRF Connect for VS Code** extension.

2. Nordic nRF Connect SDK (`v3.4.1` tested).

3. Compatible Adafruit UF2 Bootloader pre-flashed on the board (`nice!nano 0.11.0` tested).

### SDK Path Variable

Set the `NCS_DIR` environment variable to match your installation path:

```powershell
$env:NCS_DIR = "C:\ncs\v3.4.1"
```

Verify paths in PowerShell:

```powershell
Test-Path "$env:NCS_DIR\modules\lib\matter\scripts\tools\zap\zap_download.py"
Test-Path "$env:NCS_DIR\zephyr\scripts\build\uf2conv.py"
```

Both commands should output `True`.

## 5. Creating Project from Sample

1. Open VS Code and navigate to the **nRF Connect** tab.

2. Select SDK `v3.4.1`.

3. Use **Create a new application** from sample choosing the Nordic Matter Contact Sensor.

4. Save the copy outside the SDK directory, e.g.:

   ```text
   %USERPROFILE%\nrf\<PROJECT NAME>
   ```

## 6. Devicetree Overlays Configuration

To ensure both the primary application and the MCUboot child image share the same hardware modifications and flash layout, create identical overlay files at both locations:

1. **Application Overlay:** `boards/nrf52840dk_nrf52840.overlay`

2. **MCUboot Overlay:** `sysbuild/mcuboot/boards/nrf52840dk_nrf52840.overlay`

### Overlay Content

```dts
/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <samples/matter/nrf52840_partitions.dtsi>

/ {
	chosen {
		zephyr,code-partition = &boot_partition;
		//nordic,pm-ext-flash = &mx25r64;
	};

	leds {
		led0: led_0 {
			gpios = <&gpio0 15 GPIO_ACTIVE_HIGH>;
			label = "RED LED";
		};
		led1: led_1 {
			gpios = <&gpio0 24 GPIO_ACTIVE_HIGH>;
			label = "hal pw";
		};
		led2: led_2 {
			gpios = <&gpio0 11 GPIO_ACTIVE_HIGH>;
			label = "pir pw";
		};
		led3: led_3 {
			gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
			label = "ext pw";
		};
	};

	/* Reconfigure Buttons */
	buttons {
		compatible = "gpio-keys";
		button0: button_0 {
			gpios = <&gpio0 17 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
			label = "hal sensor";
			zephyr,code = <17>;
		};
		button1: button_1 {
			gpios = <&gpio0 20 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
			label = "pir sensor";
			zephyr,code = <20>;
		};
		button2: button_2 {
			gpios = <&gpio0 22 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
			label = "Factory Reset";
			zephyr,code = <22>;
		};

		/* Remove the 4th button */
		/delete-node/ button_3;
	};

	aliases {
		// /delete-property/ led0;
		/delete-property/ led1;
		/delete-property/ led2;
		/delete-property/ led3;

		/delete-property/ pwm-led0;
		/delete-property/ mcuboot-button0;
		/delete-property/ mcuboot-led0;
		/delete-property/ sw0;
		/delete-property/ sw1;
		/delete-property/ sw2;
		/delete-property/ sw3;
	};
};

/delete-node/ &pwm_led0;

&mx25r64 {
	status = "disabled";
};

&qspi {
    status = "disabled";
    /delete-node/ mx25r6435f@0;
};

/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &factory_data_partition;
/delete-node/ &storage_partition;

&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        boot_partition: partition@1000 {
            compatible = "zephyr,mapped-partition";
            label = "mcuboot";
            reg = <0x00001000 DT_SIZE_K(28)>;
        };

        slot0_partition: partition@8000 {
            compatible = "zephyr,mapped-partition";
            label = "image-0";
            reg = <0x00008000 DT_SIZE_K(896)>;
        };

        slot1_partition: partition@e8000 {
            compatible = "zephyr,mapped-partition";
            label = "image-1";
            reg = <0x000e8000 DT_SIZE_K(4)>;
        };

        factory_data_partition: partition@e9000 {
            compatible = "zephyr,mapped-partition";
            label = "factory-data";
            reg = <0x000e9000 DT_SIZE_K(4)>;
        };

        storage_partition: partition@ea000 {
            compatible = "zephyr,mapped-partition";
            label = "storage";
            reg = <0x000ea000 DT_SIZE_K(40)>;
        };
    };
};
```
### Why the overlay differs from the DK sample

-   The ProMicro does not have the external QSPI flash used by the DK
    configuration.
-   The Adafruit bootloader reset circuit involves P0.18, P0.16, and
    P0.14 according to the board design.
-   The DK configuration may use one of the relevant pins for an LED,
    causing a reset loop unless adjusted.
-   The overlay disables the external QSPI flash, removes DK-specific
    LED/button aliases that are not used, reconfigures the buttons and
    LEDs, and defines the internal flash partitions.
    
## 7. Software Trigger for Adafruit UF2 Bootloader Mode

Since physical access to reset pins may be limited on custom hardware, the application logic should include a mechanism to trigger the Adafruit UF2 bootloader directly from software logic.

Write `0x57` to the Retention Register `GPREGRET` and perform a system reset:

```c
#include <nrfx.h>

void trigger_adafruit_dfu_bootloader(void)
{
    /* Write Adafruit UF2 DFU magic flag to GPREGRET */
    NRF_POWER->GPREGRET = 0x57;

    /* Trigger System Reset into Adafruit Bootloader */
    NVIC_SystemReset();
}
```

When rebooting, the Adafruit bootloader detects `0x57` in `GPREGRET`, enters UF2 mass storage bootloader mode, and mounts as a USB drive.

## 8. ZAP Tool Workflow

NCS Helper:

``` text
%NCS_DIR%\modules\lib\matter\scripts\tools\zap
```

The `zap_download.py` script downloads the ZAP version that matches the
Matter source tree in use. Check the script's usage for the exact NCS
revision, for example:

``` powershell
python "$env:NCS_DIR\modules\lib\matter\scripts\tools\zap\zap_download.py" --help
```

After downloading and extracting ZAP, determine the actual path to
`zap.exe`. In the example below, replace `<ZAP_EXE>` with the full path
to that file.
### 8.1. Launching ZAP Tool

Launch ZAP using matching templates from the current NCS installation:

```cmd
"<ZAP_EXE>" --logToStdout ^
  --gen "%NCS_DIR%\modules\lib\matter\src\app\zap-templates\app-templates.json" ^
  --zcl "%NCS_DIR%\modules\lib\matter\src\app\zap-templates\zcl\zcl.json" ^
  -i "%PROJECT_DIR%\src\default_zap\contact_sensor.zap"
```

### 8.2. Code Generation Scripts

Create `zap-gen.cmd` in the project root:

```cmd
python "<NCS_DIR>/modules/lib/matter/scripts/tools/zap/generate.py"	 --templates "C:/ncs/v3.4.1/modules/lib/matter/src/app/zap-templates/app-templates.json" --zcl "<NCS_DIR>/modules/lib/matter/src/app/zap-templates/zcl/zcl.json" --output-dir "%CD%/src/default_zap/zap-generated" "%CD%/src/default_zap/contact_sensor.zap"
pause
```

Create `zap-matter-gen.cmd` in the project root:

```cmd
python <NCS_DIR>\modules\lib\matter\scripts\tools\zap\generate.py "%CD%\src\default_zap\contact_sensor.zap" -o "%CD%\src\default_zap"
pause
```

Execution steps after modifying ZAP configuration:

1. Save changes in ZAP UI.
2. Run `zap-gen.cmd`.
3. Run `zap-matter-gen.cmd`.
4. Rebuild project.

## 9. Building Firmware

Build the firmware target using `west`:

```powershell
Remove-Item -Recurse -Force build 
west build -p -b nrf52840dk/nrf52840
```

Expected output build binaries:

```text
build/mcuboot/zephyr/zephyr.hex
build/matter_factory_data/zephyr/factory_data.hex
build/<PROJECT NAME>/zephyr/zephyr.signed.hex
```

## 10. Merging HEX Files and UF2 Conversion

Create `make-uf2.cmd` at the project root to bundle artifacts into a single `.uf2` file:

```cmd
python -c "from intelhex import IntelHex; ih = IntelHex('build/mcuboot/zephyr/zephyr.hex'); ih.loadhex('build/matter_factory_data/zephyr/factory_data.hex'); ih.loadhex('build/<PROJECT NAME>/zephyr/zephyr.signed.hex'); ih.write_hex_file('uf2.hex')" 
pause 
python <NCS_DIR>/zephyr/scripts/build/uf2conv.py uf2.hex --family 0xADA52840 --convert --output uf2.uf2
pause
```

Required Python library dependency:

```powershell
python -m pip install intelhex
```

## 11. Flashing Firmware via USB

1. Connect nRF52840 ProMicro via USB.
2. Enter Adafruit UF2 bootloader mode (via double-resetting the reset pin or calling the software trigger function).
3. Drag and drop `uf2.uf2` into the mounted mass storage volume.
4. Device will automatically reboot and start Matter application.

## 12. Troubleshooting

* **ZAP generation failures:** Verify script paths point to correct file locations and `NCS_DIR` is set correctly.
* **Boot failures:** Verify both `boards/` and `sysbuild/mcuboot/boards/` overlay files match and define identical partition sizes.


## 13. Known Limitations

* Dual-slot Matter OTA is not supported (`slot1` is allocated as a minimal placeholder).
* Firmware updates must be applied manually over USB UF2.
* Device identifies as `nRF52840-nicenano` in Adafruit UF2 Bootloader context.
* In this setup, I didn't use softdevice so BLE DFU not support, you can try to modify flash layout start address from 0x1000 to 0x26000 to keep softdevie and initiate BLE DFU by application logic `NRF_POWER->GPREGRET = 0xA8; // BLE DFU, 0x57 USB DFU, 0x4e Serial`
