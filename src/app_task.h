/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};
	CHIP_ERROR StartApp();

private:
	CHIP_ERROR Init();
	static uint8_t sResetStage;
	static struct k_work_delayable sFactoryResetWork;
	static struct k_work_delayable sSensor1Work;
	static void FactoryResetIsrHandler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
    static void FactoryResetTimerHandler(struct k_work *work);
	static void sensor1_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
	static void Sensor1TimerHandler(struct k_work *work);
	int InitCustomButtons(void);
};
