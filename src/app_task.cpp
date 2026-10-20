/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include "lib/core/CHIPError.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/boolean-state-server/CodegenIntegration.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr chip::EndpointId kContactSensorEndpointId = 1;
Nrf::Matter::IdentifyCluster sIdentifyCluster(kContactSensorEndpointId);
//

static const struct gpio_dt_spec sensor1 = GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);
static const struct gpio_dt_spec reset_button = GPIO_DT_SPEC_GET(DT_NODELABEL(button2), gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static struct gpio_callback sensor1_cb_data;
static struct gpio_callback reset_cb_data;

} /* namespace */

struct k_work_delayable AppTask::sSensor1Work;
struct k_work_delayable AppTask::sFactoryResetWork;
uint8_t AppTask::sResetStage = 0;
void AppTask::sensor1_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    gpio_pin_interrupt_configure_dt(&sensor1, GPIO_INT_DISABLE);
    k_work_schedule(&sSensor1Work, K_MSEC(300));
}
void AppTask::Sensor1TimerHandler(struct k_work *work)
{
    int val = gpio_pin_get_dt(&sensor1);
    Nrf::PostTask([val] {
			auto booleanState =
				app::Clusters::BooleanState::FindClusterOnEndpoint(kContactSensorEndpointId);
			if (booleanState == nullptr) {
				LOG_ERR("Updating contact sensor state failed");
				return;
			}
			booleanState->SetStateValue(val);
		});
	gpio_pin_interrupt_configure_dt(&sensor1, (val == 1) ? GPIO_INT_LEVEL_INACTIVE : GPIO_INT_LEVEL_ACTIVE);
}
void AppTask::FactoryResetIsrHandler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_interrupt_configure_dt(&reset_button, GPIO_INT_DISABLE);
    int val = gpio_pin_get_dt(&reset_button);
    if (val == 1) {
        //LOG_INF("Reset button pressed. Starting Stage 1 (3s)...");
        sResetStage = 0;
        k_work_schedule(&sFactoryResetWork, K_MSEC(3000));
        gpio_pin_interrupt_configure_dt(&reset_button, GPIO_INT_LEVEL_INACTIVE);
    } else {
        //LOG_INF("Reset button released. Canceling reset process.");
        k_work_cancel_delayable(&sFactoryResetWork);
        sResetStage = 0;
        gpio_pin_set_dt(&led0, 0);
        gpio_pin_interrupt_configure_dt(&reset_button, GPIO_INT_LEVEL_ACTIVE);
    }
}
void AppTask::FactoryResetTimerHandler(struct k_work *work)
{
    if (sResetStage == 0) {
        LOG_INF("Stage 1 reached: Turning ON LED warning...");
        gpio_pin_set_dt(&led0, 1);
        sResetStage = 1;
        k_work_reschedule(&sFactoryResetWork, K_MSEC(5000));
        
    } else if (sResetStage == 1) {
        LOG_INF("Stage 2 reached: Turning OFF LED and Resetting...");
        gpio_pin_set_dt(&led0, 0);
        sResetStage = 0;
        chip::Server::GetInstance().ScheduleFactoryReset();
    }
}
int AppTask::InitCustomButtons(void)
{
    //SENSOR 1 (Push-Pull: Floating / No Pull) ---
    gpio_pin_configure_dt(&sensor1, GPIO_INPUT| GPIO_PULL_UP);
    int ret =gpio_pin_interrupt_configure_dt(&sensor1, GPIO_INT_LEVEL_ACTIVE);
    if (ret) {
        LOG_ERR("Failed to configure interrupt: %d", ret);
        return ret;
    }
    gpio_init_callback(&sensor1_cb_data, sensor1_isr_handler, BIT(sensor1.pin));
    gpio_add_callback(sensor1.port, &sensor1_cb_data);

	//FACTORY RESET (Pull-Up)
    gpio_pin_configure_dt(&reset_button, GPIO_INPUT| GPIO_PULL_UP);
    ret = gpio_pin_interrupt_configure_dt(&reset_button, GPIO_INT_LEVEL_ACTIVE);
    if (ret) {
        LOG_ERR("Failed to configure interrupt: %d", ret);
        return ret;
    }
    gpio_init_callback(&reset_cb_data, FactoryResetIsrHandler, BIT(reset_button.pin));
    gpio_add_callback(reset_button.port, &reset_cb_data);

	//output
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    LOG_INF("Custom Buttons initialized successfully");
    return 0;
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());
    k_work_init_delayable(&sSensor1Work, AppTask::Sensor1TimerHandler);
	k_work_init_delayable(&sFactoryResetWork, AppTask::FactoryResetTimerHandler);
	if (InitCustomButtons() != 0) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}
	ReturnErrorOnFailure(sIdentifyCluster.Init());
	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{

	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
