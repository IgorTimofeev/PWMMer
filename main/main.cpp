#include <algorithm>
#include <cmath>

#include "encoder.h"
#include "esp_log.h"
#include "constants.h"
#include "encoder.h"
#include "driver/ledc.h"
#include "motor.h"

using namespace pizda;

Encoder encoder {
	constants::encoder::dt,
	constants::encoder::slk,
	constants::encoder::sw
};

Motor motor {
	constants::signal,
	LEDC_CHANNEL_0,
	1000,
	2000,
	50
};

uint8_t percent = 0;
bool percentMode = true;

void updatePulseWidthRange() {
	motor.setMaxPulseWidth(3000 - motor.getMinPulseWidth());

	ESP_LOGI("Main", "updatePulseWidthRange(): %f - %f", (float) motor.getMinPulseWidth(), (float) motor.getMaxPulseWidth());
}

void updatePercent() {
	motor.setPercent(percent);

	ESP_LOGI("Main", "setPercent() percent: %f", (float) percent);
}

extern "C" void app_main(void) {
	// Motor
	motor.setup();

	updatePulseWidthRange();
	updatePercent();

	// Encoder
	encoder.setup();

	// ReSharper disable once CppDFAEndlessLoop
	while (true) {
		if (encoder.wasInterrupted()) {
			encoder.acknowledgeInterrupt();

			if (encoder.isPressed()) {
				ESP_LOGI("Main", "Mode: %s", percentMode ? "percent" : "pulse width");
				percentMode = !percentMode;

				// ESP_LOGI("Main", "Calibrating");
				//
				// motor.setPulseWidth(motor.getMaxPulseWidth());
				// vTaskDelay(pdMS_TO_TICKS(3'000));
				//
				// motor.setPulseWidth(motor.getMinPulseWidth());
			}
			else {
				if (std::abs(encoder.getRotation()) > 3) {
					if (percentMode) {
						percent = static_cast<uint8_t>(std::clamp<int16_t>(
							static_cast<int16_t>(percent + (encoder.getRotation() > 0 ? 5 : -5)),
							0,
							100
						));

						updatePercent();
					}
					else {
						motor.setMinPulseWidth(static_cast<uint16_t>(std::clamp<int32_t>(
							static_cast<int32_t>(motor.getMinPulseWidth()) + (encoder.getRotation() > 0 ? 50 : -50),
							100,
							1500 - 100
						)));

						updatePulseWidthRange();
					}

					encoder.setRotation(0);
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(1'000 / 30));
	}
}
