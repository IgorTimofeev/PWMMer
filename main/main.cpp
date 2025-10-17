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
	constants::encoder::b,
	constants::encoder::a,
	constants::encoder::sw
};

// Motor
Motor motor {
	constants::signal,
	LEDC_CHANNEL_0
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

MonochromeColor colorB { false };
MonochromeColor colorF { true };
PIXY10Font fontBig {};
PIXY10Font fontSmall {};

bool modeMode = false;
uint8_t percent = 0;
uint16_t pulseWidth = 0;

void updateMaxPulseWidth() {
	motor.setMaxPulseWidth(3000 - settings.minPulseWidth);

	ESP_LOGI("Main", "updateMaxPulseWidth(): %f - %f", (float) settings.minPulseWidth, (float) motor.getMaxPulseWidth());
}

void encoderTick() {
	if (!encoder.wasInterrupted())
		return;

	encoder.acknowledgeInterrupt();

	if (encoder.isPressed()) {
		modeMode = !modeMode;
		display.setInverted(modeMode);
	}
	else {
		const auto absRotation = std::abs(encoder.getRotation());

		if (absRotation < 4)
			return;

//		ESP_LOGI("Main", "Rotation: %d", encoder.getRotation());

		if (modeMode) {
			settings.mode = static_cast<SettingsMode>(std::clamp<int8_t>(
				static_cast<int8_t>(settings.mode) + (encoder.getRotation() > 0 ? 1 : -1),
				0,
				static_cast<int8_t>(SettingsMode::last)
			));

			settings.scheduleWrite();

			ESP_LOGI("Main", "Mode: %d", static_cast<int8_t>(settings.mode));
		}
		else {
			const auto magnitudeBig = absRotation >= 5;

			switch (settings.mode) {
				case SettingsMode::pulseWidth: {
					const auto magnitude = magnitudeBig ? 50 : 10;

					pulseWidth = static_cast<uint16_t>(std::clamp<int32_t>(
						static_cast<int32_t>(pulseWidth + (encoder.getRotation() > 0 ? magnitude : -magnitude)),
						settings.minPulseWidth,
						motor.getMaxPulseWidth()
					));

					percent = (pulseWidth - settings.minPulseWidth) * 100 / (motor.getMaxPulseWidth() - settings.minPulseWidth);

					motor.setPulseWidth(pulseWidth);

					break;
				}
				case SettingsMode::percent: {
					const auto magnitude = magnitudeBig ? 5 : 1;

					percent = static_cast<uint8_t>(std::clamp<int16_t>(
						static_cast<int16_t>(percent + (encoder.getRotation() > 0 ? magnitude : -magnitude)),
						0,
						100
					));

					pulseWidth = settings.minPulseWidth + (motor.getMaxPulseWidth() - settings.minPulseWidth) * percent / 100;

					motor.setPulseWidth(pulseWidth);

					break;
				}
				default: {
					const auto magnitude = magnitudeBig ? 50 : 10;

					settings.minPulseWidth = static_cast<uint16_t>(std::clamp<int32_t>(
						static_cast<int32_t>(settings.minPulseWidth) + (encoder.getRotation() > 0 ? magnitude : -magnitude),
						10,
						1500 - 10
					));

					updateMaxPulseWidth();

					// Just for safety
					pulseWidth = settings.minPulseWidth;
					motor.setMinPulseWidth(settings.minPulseWidth);

					settings.scheduleWrite();

					break;
				}
			}
		}

		encoder.setRotation(0);
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

	switch (settings.mode) {
		case SettingsMode::pulseWidth: {
			unitsMin = settings.minPulseWidth;
			unitsMax = motor.getMaxPulseWidth();
			unitsValue = pulseWidth;

			unitsStep0 = 10;
			unitsStep1 = 50;
			unitsStep2 = 100;
			radPerUnit = toRadians(0.2f);

			modeText = L"Pulse width";

			break;
		}
		case SettingsMode::percent: {
			unitsMin = 0;
			unitsMax = 100;
			unitsValue = static_cast<uint16_t>(percent);

			unitsStep0 = 1;
			unitsStep1 = 5;
			unitsStep2 = 10;
			radPerUnit = toRadians(1.5f);

			modeText = L"Percent";

			break;
		}
		default: {
			unitsMin = 0;
			unitsMax = 1500;
			unitsValue = settings.minPulseWidth;

			unitsStep0 = 10;
			unitsStep1 = 50;
			unitsStep2 = 100;
			radPerUnit = toRadians(0.2f);

			modeText = L"Range min";

			break;
		}
	}

	renderer.clear(&colorB);

	// Rose
	{
		// Triangle
		const uint8_t triangleWidth = 6;
		const uint8_t triangleHeight = 4;

		renderer.renderFilledTriangle(
			Point(resolution.getWidth() / 2 - triangleWidth / 2, 0),
			Point(resolution.getWidth() / 2 + triangleWidth / 2, 0),
			Point(resolution.getWidth() / 2, triangleHeight),
			&colorF
		);

		const auto roseMarginTop = 3;
		const auto roseRadius = resolution.getHeight() * 2 + 32;

		const auto pivot = Point(
			resolution.getWidth() / 2,
			triangleHeight + roseMarginTop + roseRadius
		);

		// Lines
		for (uint16_t arrowUnits = unitsMin; arrowUnits <= unitsMax; arrowUnits += unitsStep0) {
			const auto isBig = arrowUnits % unitsStep2 == 0;
			const uint8_t arrowLength = isBig ? 5 : (arrowUnits % unitsStep1 == 0 ? 3 : 1);

			const auto arrowVecNorm =
				Vector2F(0, -1)
				.rotate(
					(
						static_cast<float>(arrowUnits)
						- static_cast<float>(unitsValue)
					)
					* radPerUnit
				);
			const auto arrowVecTo = arrowVecNorm * static_cast<float>(roseRadius);
			const auto arrowVecFrom = arrowVecTo - arrowVecNorm * static_cast<float>(arrowLength);

			renderer.renderLine(
				pivot + (Point) arrowVecFrom,
				pivot + (Point) arrowVecTo,
				&colorF
			);

			if (isBig) {
				const auto text = std::to_wstring(arrowUnits);
				const auto textSizeVec = Vector2F(fontBig.getWidth(text), fontBig.getHeight());
				const auto textMargin = 2;
				const auto textVec = arrowVecFrom - arrowVecNorm * (textSizeVec.getLength() / 2 + textMargin);

				renderer.renderString(
					Point(
						pivot.getX() + (int32_t) (textVec.getX() - textSizeVec.getX() / 2),
						pivot.getY() + (int32_t) (textVec.getY() - textSizeVec.getY() / 2)
					),
					&fontBig,
					&colorF,
					text
				);
			}
		}
	}

	// Mode name
	{
		// Text
		const auto modeTextWidth = fontSmall.getWidth(modeText);

		const auto modeTextPos = Point(
			resolution.getWidth() / 2 - modeTextWidth / 2,
			resolution.getHeight() - fontSmall.getHeight() + 1
		);

		renderer.renderString(
			modeTextPos,
			&fontSmall,
			&colorF,
			modeText
		);

		// Dots
		constexpr static uint8_t dotSpacing = 5;
		const auto dotY = modeTextPos.getY() + fontSmall.getHeight() / 2;

		const auto renderDots = [dotY](int32_t x, uint8_t count) {
			for (uint8_t i = 0; i < count; i++) {
				renderer.renderPixel(Point(x, dotY), &colorF);
				x += dotSpacing;
			}
		};

		const auto numericMode = static_cast<uint8_t>(settings.mode);

		// Left
		if (settings.mode != SettingsMode::first)
			renderDots(modeTextPos.getX() - numericMode * dotSpacing, numericMode);

		// Right
		if (settings.mode != SettingsMode::last)
			renderDots(modeTextPos.getX() + modeTextWidth - 1 + dotSpacing, static_cast<uint8_t>(SettingsMode::last) - numericMode);
	}

	renderer.flush();
}

void calibrate() {
	renderer.clear(&colorF);

	const auto text = std::format(L"Calibration", settings.minPulseWidth, motor.getMaxPulseWidth());

	renderer.renderString(
		Point(
			display.getSize().getWidth() / 2 - fontBig.getWidth(text) / 2,
			display.getSize().getHeight() / 2 - fontBig.getHeight() / 2
		),
		&fontBig,
		&colorB,
		text
	);

	renderer.flush();

	// Init
	motor.setPulseWidth(0);
	vTaskDelay(pdMS_TO_TICKS(2'000));

	// Max
	motor.setPulseWidth(motor.getMaxPulseWidth());
	vTaskDelay(pdMS_TO_TICKS(1'000));

	// Min
	pulseWidth = settings.minPulseWidth;
	motor.setPulseWidth(pulseWidth);
	vTaskDelay(pdMS_TO_TICKS(4'000));
}

extern "C" void app_main(void) {
	// Settings
	settings.setup();
	settings.read();

	// Encoder
	encoder.setup();

	// Motor
	motor.setup();

	motor.setMinPulseWidth(settings.minPulseWidth);
	updateMaxPulseWidth();

	// Display
	display.setup();
	renderer.setTarget(&display);

	// Calibration
	if (encoder.isPressed()) {
		calibrate();

		encoder.acknowledgeInterrupt();
	}
	else {
		pulseWidth = settings.minPulseWidth;
		motor.setPulseWidth(pulseWidth);
	}

	while (true) {
		encoderTick();
		displayTick();

		vTaskDelay(pdMS_TO_TICKS(1'000 / 30));
	}
}
