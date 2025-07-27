#pragma once

#include "driver/ledc.h"
#include "cstdint"
#include "esp_adc/adc_oneshot.h"
#include "driver/uart.h"

namespace pizda {
	class constants {
		public:
			class i2c {
				public:
					constexpr static gpio_num_t sda = GPIO_NUM_8;
					constexpr static gpio_num_t scl = GPIO_NUM_9;
			};

			class screen {
				public:
					constexpr static uint8_t address = 12;
					constexpr static uint32_t frequency = 60'000'000;
			};

			class encoder {
				public:
					constexpr static gpio_num_t sw = GPIO_NUM_3;
					constexpr static gpio_num_t slk = GPIO_NUM_2;
					constexpr static gpio_num_t dt = GPIO_NUM_1;
			};

			constexpr static gpio_num_t signal = GPIO_NUM_0;
	};
}
