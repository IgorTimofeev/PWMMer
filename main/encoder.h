#pragma once

#include "driver/gpio.h"

class Encoder {
	public:
		Encoder(const gpio_num_t& clkPin, const gpio_num_t& dtPin, const gpio_num_t& swPin) :
			_clkPin(clkPin),
			_dtPin(dtPin),
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

			gpio_isr_handler_add(_clkPin, onClkOrDtInterrupt, this);
			gpio_isr_handler_add(_dtPin, onClkOrDtInterrupt, this);
			gpio_isr_handler_add(_swPin, onSwInterrupt, this);

			// Updating initial values
			_pressed = readSw();
			_oldClkAndDt = readClkAndDt();
		}

		static void onClkOrDtInterrupt(void* arg) {
			const auto encoder = static_cast<Encoder*>(arg);

			encoder->readRotation();
			encoder->_interrupted = true;
		}

		static void onSwInterrupt(void* arg) {
			const auto encoder = static_cast<Encoder*>(arg);

			encoder->readPressed();
			encoder->_interrupted = true;
		}

		bool wasInterrupted() const {
			return _interrupted;
		}

		void acknowledgeInterrupt() {
			_interrupted = false;
		}

		bool isPressed() const {
			return _pressed;
		}

		int16_t getRotation() const {
			return _rotation;
		}

		void setRotation(int16_t value) {
			_rotation = value;
		}

	private:
		gpio_num_t
			_clkPin,
			_dtPin,
			_swPin;

		int16_t _rotation = 0;
		bool _pressed = false;
		bool _interrupted = false;
		uint8_t _oldClkAndDt = 0;

		uint8_t readClkAndDt() const {
			return (gpio_get_level(_clkPin) << 1) | gpio_get_level(_dtPin);
		}

		bool readSw() const {
			return !gpio_get_level(_swPin);
		}

		void readPressed() {
			_pressed = readSw();
		}

		void readRotation() {
			const auto clkAndDt = readClkAndDt();

			if (clkAndDt != _oldClkAndDt) {
				switch (_oldClkAndDt | (clkAndDt << 2)) {
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

				_oldClkAndDt = clkAndDt;
			}
		}
};
