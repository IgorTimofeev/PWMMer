#pragma once

class Motor {
	public:
		Motor(
			const gpio_num_t pin,
			const ledc_channel_t channel,
			const uint16_t maxAngle,
			const uint16_t minPulseWidthUs = 1000,
			const uint16_t maxPulseWidthUs = 2000,
			const uint16_t frequencyHz = 50
		) :
			pin(pin),
			channel(channel),
			maxAngle(maxAngle),
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

		uint16_t getMaxAngle() const {
			return maxAngle;
		}

		uint16_t getAngle() const {
			return angle;
		}

		void setAngle(const uint16_t value) {
			angle = value;

			const auto duty = (minPulseWidthUs + (maxPulseWidthUs - minPulseWidthUs) * angle / maxAngle) * dutyResolutionMaxValue / (1'000'000 / frequencyHz);

			ESP_LOGI("Motor", "degree: %f, duty: %f", (float) angle, (float) duty);

			ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
			ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));
		}

	private:
		gpio_num_t pin;
		ledc_channel_t channel;
		uint16_t maxAngle;
		uint16_t minPulseWidthUs;
		uint16_t maxPulseWidthUs;
		uint16_t frequencyHz;
		uint16_t angle = 0;

		constexpr static ledc_timer_bit_t dutyResolution = LEDC_TIMER_13_BIT;
		constexpr static uint16_t dutyResolutionMaxValue = static_cast<uint16_t>(std::pow(2, static_cast<uint8_t>(dutyResolution))) - 1;
};