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
	180,
	500,
	2500,
	50
};

extern "C" void app_main(void) {
	// Motor
	motor.setup();
	motor.setAngle(motor.getMaxAngle() / 2);

	// Encoder
	encoder.setup();

	// ReSharper disable once CppDFAEndlessLoop
	while (true) {
		if (encoder.wasInterrupted()) {
			encoder.acknowledgeInterrupt();

			if (std::abs(encoder.getRotation()) > 3) {
				motor.setAngle(static_cast<uint16_t>(std::clamp<int32_t>(
					static_cast<int32_t>(motor.getAngle()) + (encoder.getRotation() > 0 ? 10 : -10),
					0,
					motor.getMaxAngle()
				)));

				encoder.setRotation(0);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(1'000 / 30));
	}
}
