#pragma once

#include "driver/ledc.h"
#include "cstdint"
#include "esp_adc/adc_oneshot.h"
#include "driver/uart.h"

namespace pizda {
	class constants {
		public:
			class spi {
				public:
					constexpr static gpio_num_t mosi = GPIO_NUM_10;
					constexpr static gpio_num_t clock = GPIO_NUM_8;
			};

			class screen {
				public:
					constexpr static gpio_num_t dataCommand = GPIO_NUM_6;
					constexpr static gpio_num_t slaveSelect = GPIO_NUM_5;
					constexpr static gpio_num_t reset = GPIO_NUM_7;
					constexpr static uint32_t frequency = 10'000'000;
			};

			class encoder {
				public:
					constexpr static gpio_num_t sw = GPIO_NUM_0;
					constexpr static gpio_num_t a = GPIO_NUM_1;
					constexpr static gpio_num_t b = GPIO_NUM_2;
			};

			constexpr static gpio_num_t signal = GPIO_NUM_3;
	};
}
