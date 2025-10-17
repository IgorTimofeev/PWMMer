#pragma once

#include <YOBANVS/main.h>

namespace pizda {
	using namespace YOBA;

	enum class SettingsMode : uint8_t {
		pulseWidth,
		percent,
		minPulseWidth,

		first = pulseWidth,
		last = minPulseWidth
	};

	class Settings : public NVSSettings {
		public:
			SettingsMode mode = SettingsMode::pulseWidth;
			uint16_t minPulseWidth = 1000;

		protected:
			const char* getNVSNamespace() override {
				return "st2";
			}

			void onRead(const NVSStream& stream) override {
				mode = static_cast<SettingsMode>(stream.getUint8(_mode, static_cast<uint8_t>(SettingsMode::pulseWidth)));
				minPulseWidth = stream.getUint16(_minPulseWidth, 1'000);
			}

			void onWrite(const NVSStream& stream) override {
				stream.setUint8(_mode, static_cast<uint8_t>(mode));
				stream.setUint16(_minPulseWidth, minPulseWidth);
			}

			private:
				constexpr static auto _mode = "md";
				constexpr static auto _minPulseWidth = "pw";
		};
}