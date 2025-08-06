#pragma once

namespace pizda {
	class Motor {
		public:
			Motor(
				const gpio_num_t pin,
				const ledc_channel_t channel,
				const uint16_t minPulseWidthUs = 1000,
				const uint16_t maxPulseWidthUs = 2000,
				const uint16_t frequencyHz = 50
			) :
				pin(pin),
				channel(channel),
				minPulseWidthUs(minPulseWidthUs),
				maxPulseWidthUs(maxPulseWidthUs),
				frequencyHz(frequencyHz)
			{

			}

			void setup() const {
				ledc_timer_config_t timerConfig {};
				timerConfig.speed_mode       = LEDC_LOW_SPEED_MODE;
				timerConfig.duty_resolution  = dutyResolution;
				timerConfig.timer_num        = LEDC_TIMER_0;
				timerConfig.freq_hz          = frequencyHz;
				timerConfig.clk_cfg          = LEDC_AUTO_CLK;
				ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

				ledc_channel_config_t channelConfig {};
				channelConfig.speed_mode     = LEDC_LOW_SPEED_MODE;
				channelConfig.channel        = channel;
				channelConfig.timer_sel      = LEDC_TIMER_0;
				channelConfig.intr_type      = LEDC_INTR_DISABLE;
				channelConfig.gpio_num       = pin;
				channelConfig.duty           = 0;
				channelConfig.hpoint         = 0;
				ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));
			}

			uint16_t getMinPulseWidth() const {
				return minPulseWidthUs;
			}

			void setMinPulseWidth(const uint16_t minPulseWidthUs) {
				this->minPulseWidthUs = minPulseWidthUs;
			}

			uint16_t getMaxPulseWidth() const {
				return maxPulseWidthUs;
			}

			void setMaxPulseWidth(const uint16_t maxPulseWidthUs) {
				this->maxPulseWidthUs = maxPulseWidthUs;
			}

			uint16_t getFrequency() const {
				return frequencyHz;
			}

			void setFrequency(const uint16_t frequencyHz) {
				this->frequencyHz = frequencyHz;
			}

			void setPulseWidth(const uint16_t valueUs) const {
				const auto duty = valueUs * dutyResolutionMaxValue / (1'000'000 / frequencyHz);

				// ESP_LOGI("Motor", "setPulse() duty: %f", (float) duty);

				ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
				ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
			}

			void setPercent(const uint8_t percent) const {
				setPulseWidth(minPulseWidthUs + (maxPulseWidthUs - minPulseWidthUs) * percent / 100);
			}

		private:
			gpio_num_t pin;
			ledc_channel_t channel;
			uint16_t minPulseWidthUs;
			uint16_t maxPulseWidthUs;
			uint16_t frequencyHz;

			constexpr static ledc_timer_bit_t dutyResolution = LEDC_TIMER_13_BIT;
			constexpr static uint16_t dutyResolutionMaxValue = static_cast<uint16_t>(std::pow(2, static_cast<uint8_t>(dutyResolution))) - 1;
	};

	class Servo : public Motor {
		public:
			Servo(
				const gpio_num_t pin,
				const ledc_channel_t channel,
				const uint16_t maxAngle,
				const uint16_t minPulseWidthUs = 1000,
				const uint16_t maxPulseWidthUs = 2000,
				const uint16_t frequencyHz = 50
			) :
				Motor(
					pin,
					channel,
					minPulseWidthUs,
					maxPulseWidthUs,
					frequencyHz
				),
				maxAngle(maxAngle)
			{

			}

			uint16_t getMaxAngle() const {
				return maxAngle;
			}

			void setMaxAngle(const uint16_t maxAngle) {
				this->maxAngle = maxAngle;
			}

			void setAngle(const uint16_t angle) const {
				const auto pulse = getMinPulseWidth() + (getMaxPulseWidth() - getMinPulseWidth()) * angle / maxAngle;

				// ESP_LOGI("Motor", "setAngle() pulse: %f", (float) pulse);

				setPulseWidth(pulse);
			}

		private:
			uint16_t maxAngle;
	};
}