#pragma once

#include <esp_timer.h>
#include "driver/gpio.h"

class Encoder {
	public:
		Encoder(const gpio_num_t& aPin, const gpio_num_t& bPin, const gpio_num_t& swPin) :
			_clkPin(aPin),
			_dtPin(bPin),
			_swPin(swPin)
		{

		}

		void setup() {
			gpio_config_t config {};
			config.pin_bit_mask = 1ULL << _clkPin | 1ULL << _dtPin | 1ULL << _swPin;
			config.mode = GPIO_MODE_INPUT;
			config.pull_up_en = GPIO_PULLUP_ENABLE;
			config.pull_down_en = GPIO_PULLDOWN_DISABLE;
			config.intr_type = GPIO_INTR_ANYEDGE;
			ESP_ERROR_CHECK(gpio_config(&config));

			gpio_install_isr_service(0);

			gpio_isr_handler_add(_clkPin, onRotationPinsInterrupt, this);
			gpio_isr_handler_add(_dtPin, onRotationPinsInterrupt, this);
			gpio_isr_handler_add(_swPin, onSwitchPinInterrupt, this);

			// Updating initial values
			_pressed = readSw();
			_oldABMask = readABMask();
		}

		static void onRotationPinsInterrupt(void* arg) {
			const auto encoder = static_cast<Encoder*>(arg);

			encoder->updateRotation();
		}

		static void onSwitchPinInterrupt(void* arg) {
			const auto encoder = static_cast<Encoder*>(arg);

			encoder->readPressed();
		}

		bool isPressedChanged() {
			return _pressed != _oldPressed;
		}

		bool fetchPressed() {
			_oldPressed = _pressed;

			return _pressed;
		}

		int16_t getRotation() {
			return _rotation;
		}

		int16_t fetchRPS() {
			const auto rps = _rotation * 1'000'000 / (esp_timer_get_time() - _oldRotationTime);

			ESP_LOGI("Encoder", "Rotation: %d, rps: %d", _rotation, rps);

			_rotation = 0;
			_oldRotationTime = esp_timer_get_time();

			return rps;
		}

	private:
		gpio_num_t
			_clkPin,
			_dtPin,
			_swPin;

		bool _oldPressed = false;
		bool _pressed = false;

		uint8_t _oldABMask = 0;
		int16_t _rotation = 0;
		int64_t _oldRotationTime = 0;

		uint8_t readABMask() const {
			return (gpio_get_level(_clkPin) << 1) | gpio_get_level(_dtPin);
		}

		bool readSw() const {
			return !gpio_get_level(_swPin);
		}

		void readPressed() {
			_pressed = readSw();
		}

		void updateRotation() {
			const auto ABMask = readABMask();

			if (ABMask != _oldABMask) {
				switch (_oldABMask | (ABMask << 2)) {
					case 0: case 5: case 10: case 15:
						break;
					case 1: case 7: case 8: case 14:
						_rotation++;
						break;
					case 2: case 4: case 11: case 13:
						_rotation--;
						break;
					case 3: case 12:
						_rotation += 2;
						break;
					default:
						_rotation -= 2;
						break;
				}

				_oldABMask = ABMask;
			}
		}
};
