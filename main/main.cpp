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

Servo servo {
	constants::signal,
	LEDC_CHANNEL_0,
	180,
	500,
	2500,
	50
};

uint16_t angle = 0;
bool angleMode = true;

void updatePulseWidthRange() {
	servo.setMaxPulseWidth(3000 - servo.getMinPulseWidth());

	ESP_LOGI("Main", "updatePulseWidthRange(): %f - %f", (float) servo.getMinPulseWidth(), (float) servo.getMaxPulseWidth());
}

void updateAngle() {
	servo.setAngle(angle);

	ESP_LOGI("Main", "setAngle() angle: %f", (float) angle);
}

extern "C" void app_main(void) {
	// Motor
	servo.setup();

	updatePulseWidthRange();
	updateAngle();

	// Encoder
	encoder.setup();

	// ReSharper disable once CppDFAEndlessLoop
	while (true) {
		if (encoder.wasInterrupted()) {
			encoder.acknowledgeInterrupt();

			if (encoder.isPressed()) {
				angleMode = !angleMode;

				ESP_LOGI("Main", "Mode: %s", angleMode ? "angle" : "pulse width");
			}
			else {
				if (std::abs(encoder.getRotation()) > 3) {
					if (angleMode) {
						angle = static_cast<uint16_t>(std::clamp<int32_t>(
							static_cast<int32_t>(angle) + (encoder.getRotation() > 0 ? 10 : -10),
							0,
							servo.getMaxAngle()
						));

						updateAngle();
					}
					else {
						servo.setMinPulseWidth(static_cast<uint16_t>(std::clamp<int32_t>(
							static_cast<int32_t>(servo.getMinPulseWidth()) + (encoder.getRotation() > 0 ? 50 : -50),
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
