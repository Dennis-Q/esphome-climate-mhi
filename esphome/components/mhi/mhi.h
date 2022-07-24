#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
    namespace mhi {
        // Temperature
        const uint8_t MHI_TEMP_MIN = 18; // Celsius
        const uint8_t MHI_TEMP_MAX = 30; // Celsius

        class MhiClimate : public climate_ir::ClimateIR {
            public:
                MhiClimate() : climate_ir::ClimateIR(
                    MHI_TEMP_MIN, MHI_TEMP_MAX, 1.0f, true, true,
                    std::set<climate::ClimateFanMode>{
                        climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                        climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH,
                        climate::CLIMATE_FAN_MIDDLE, climate::CLIMATE_FAN_FOCUS,
                        climate::CLIMATE_FAN_DIFFUSE
                    },
                    std::set<climate::ClimateSwingMode>{
                        climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                        climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH
                    },
                    std::set<climate::ClimatePreset>{
                        climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO,
                        climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_ACTIVITY
                    }
                ) {}

            protected:
                void transmit_state() override;
                /// Handle received IR Buffer
                bool on_receive(remote_base::RemoteReceiveData data) override;

        };
    } // namespace mhi
} // namespace esphome
