#pragma once

#include <YOBANVS/main.h>

namespace pizda {
	using namespace YOBA;

	class Settings : public NVSSettings {
		public:
			uint16_t minPulseWidth = 1000;

		protected:
			const char* getNVSNamespace() override {
				return "st1";
			}

			void onRead(const NVSStream& stream) override {
				minPulseWidth = stream.getUint16(_minPulseWidth, 1'000);
			}

			void onWrite(const NVSStream& stream) override {
				stream.setUint16(_minPulseWidth, minPulseWidth);
			}

			private:
				constexpr static auto _minPulseWidth = "pw";
		};
}