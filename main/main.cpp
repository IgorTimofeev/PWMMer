#include <algorithm>
#include <cmath>
#include <format>

#include "esp_log.h"

#include <YOBA/main.h>
#include <YOBA/UI.h>
#include <YOBA/hardware/displays/SH1106Display.h>
#include <YOBA/resources/fonts/PIXY10Font.h>

#include "constants.h"
#include "driver/ledc.h"
#include "motor.h"
#include "encoder.h"
#include "encoder.h"
#include "settings.h"

using namespace YOBA;
using namespace pizda;

// Settings
Settings settings {};

// Encoder
Encoder encoder {
	constants::encoder::dt,
	constants::encoder::slk,
	constants::encoder::sw
};

// Motor
Motor motor {
	constants::signal,
	LEDC_CHANNEL_0,
	1000,
	2000,
	50
};

// UI
SH1106Display display {
	constants::spi::mosi,
	static_cast<uint8_t>(GPIO_NUM_NC),
	constants::spi::clock,
	constants::screen::slaveSelect,
	constants::screen::dataCommand,
	constants::screen::reset,
	constants::screen::frequency
};

MonochromeRenderer renderer {};

MonochromeColor colorB {false };
MonochromeColor colorF {true };
PIXY10Font font {};

// Mode
uint8_t percent = 0;
bool percentMode = true;

void updatePulseWidthRange() {
	motor.setMaxPulseWidth(3000 - settings.minPulseWidth);

	ESP_LOGI("Main", "updatePulseWidthRange(): %f - %f", (float) settings.minPulseWidth, (float) motor.getMaxPulseWidth());
}

void updatePercent() {
	motor.setPercent(percent);

	ESP_LOGI("Main", "setPercent() percent: %f", (float) percent);
}

void encoderTick() {
	if (!encoder.wasInterrupted())
		return;

	encoder.acknowledgeInterrupt();

	if (encoder.isPressed()) {
		ESP_LOGI("Main", "Mode: %s", percentMode ? "percent" : "pulse width");
		percentMode = !percentMode;
	}
	else {
		const auto absRotation = std::abs(encoder.getRotation());

		if (absRotation >= 4) {
			ESP_LOGI("Main", "Rotation: %d", encoder.getRotation());

			const auto magnitudeBig = absRotation >= 5;

			if (percentMode) {
				const auto magnitude = magnitudeBig ? 5 : 1;

				percent = static_cast<uint8_t>(std::clamp<int16_t>(
					static_cast<int16_t>(percent + (encoder.getRotation() > 0 ? magnitude : -magnitude)),
					0,
					100
				));

				updatePercent();
			}
			else {
				const auto magnitude = magnitudeBig ? 50 : 10;

				settings.minPulseWidth = static_cast<uint16_t>(std::clamp<int32_t>(
					static_cast<int32_t>(settings.minPulseWidth) + (encoder.getRotation() > 0 ? magnitude : -magnitude),
					10,
					1500 - 10
				));

				settings.scheduleWrite();

				motor.setMinPulseWidth(settings.minPulseWidth);
				updatePulseWidthRange();
			}

			encoder.setRotation(0);
		}
	}
}

void displayTick() {
	const auto& resolution = display.getSize();

	uint16_t unitsMin;
	uint16_t unitsMax;
	uint16_t unitsStep0;
	uint16_t unitsStep1;
	uint16_t unitsStep2;
	uint16_t unitsValue;
	float radPerUnit;
	std::wstring modeText;

	if (percentMode) {
		unitsMin = 0;
		unitsMax = 100;
		unitsStep0 = 1;
		unitsStep1 = 5;
		unitsStep2 = 10;
		unitsValue = static_cast<uint16_t>(percent);
		radPerUnit = toRadians(1.5f);
		modeText = L"%";
	}
	else {
		unitsMin = 0;
		unitsMax = 1500;
		unitsStep0 = 10;
		unitsStep1 = 50;
		unitsStep2 = 100;
		unitsValue = settings.minPulseWidth;
		radPerUnit = toRadians(0.2f);
		modeText = L"us";
	}

	const float valueRad = (static_cast<float>(unitsValue) - static_cast<float>(unitsMin)) * radPerUnit;

	renderer.clear(&colorB);

	// Mode text
	renderer.renderString(
		Point(
			resolution.getWidth() / 2 - font.getWidth(modeText) / 2,
			resolution.getHeight() - font.getHeight() + 1
		),
		&font,
		&colorF,
		modeText
	);

	// Triangle
	const uint8_t triangleWidth = 6;
	const uint8_t triangleHeight = 4;

	renderer.renderFilledTriangle(
		Point(resolution.getWidth() / 2 - triangleWidth / 2, 0),
		Point(resolution.getWidth() / 2 + triangleWidth / 2, 0),
		Point(resolution.getWidth() / 2 , triangleHeight),
		&colorF
	);

	// Rose
	const auto roseMarginTop = 3;
	const auto roseRadius = resolution.getHeight() * 2 + 32;

	const auto pivot = Point(
		resolution.getWidth() / 2,
		triangleHeight + roseMarginTop + roseRadius
	);

	// Lines
	for (uint16_t arrowValue = unitsMin; arrowValue <= unitsMax; arrowValue += unitsStep0) {
		const auto isBig = arrowValue % unitsStep2 == 0;
		const uint8_t arrowLength = isBig ? 5 : (arrowValue % unitsStep1 == 0 ? 3 : 1);

		const auto arrowVecNorm = Vector2F(0, -1).rotate(static_cast<float>(arrowValue) * radPerUnit - valueRad);
		const auto arrowVecTo = arrowVecNorm * static_cast<float>(roseRadius);
		const auto arrowVecFrom = arrowVecTo - arrowVecNorm * static_cast<float>(arrowLength);

		renderer.renderLine(
			pivot + (Point) arrowVecFrom,
			pivot + (Point) arrowVecTo,
			&colorF
		);

		if (isBig) {
			const auto text = std::to_wstring(arrowValue);
			const auto textSizeVec = Vector2F(font.getWidth(text), font.getHeight());
			const auto textMargin = 2;
			const auto textVec = arrowVecFrom - arrowVecNorm * (textSizeVec.getLength() / 2 + textMargin);

			renderer.renderString(
				Point(
					pivot.getX() + (int32_t) (textVec.getX() - textSizeVec.getX() / 2),
					pivot.getY() + (int32_t) (textVec.getY() - textSizeVec.getY() / 2)
				),
				&font,
				&colorF,
				text
			);
		}
	}

	renderer.flush();
}

extern "C" void app_main(void) {
	// Settings
	settings.setup();
	settings.read();

	// Display
	display.setup();
	renderer.setTarget(&display);

	// Motor
	motor.setup();

	motor.setMinPulseWidth(settings.minPulseWidth);
	updatePulseWidthRange();
	updatePercent();

	// Encoder
	encoder.setup();

	while (true) {
		encoderTick();
		displayTick();

		vTaskDelay(pdMS_TO_TICKS(1'000 / 30));
	}
}
