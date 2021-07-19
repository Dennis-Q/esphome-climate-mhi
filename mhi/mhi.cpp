#include "mhi.h"
#include "esphome/core/log.h"

namespace esphome {
    namespace mhi {
        static const char *TAG = "mhi.climate";

        // Power
        const uint32_t MHI_OFF = 0x08;
        const uint32_t MHI_ON = 0x00;

        // Operating mode
        const uint8_t MHI_AUTO = 0x07;
        const uint8_t MHI_HEAT = 0x03;
        const uint8_t MHI_COOL = 0x06;
        const uint8_t MHI_DRY = 0x05;
        const uint8_t MHI_FAN = 0x04;

        // Fan speed
        const uint8_t MHI_FAN_AUTO = 0x0F;
        const uint8_t MHI_FAN1 = 0x0E;
        const uint8_t MHI_FAN2 = 0x0D;
        const uint8_t MHI_FAN3 = 0x0C;
        const uint8_t MHI_FAN4 = 0x0B;
        const uint8_t MHI_HIPOWER = 0x07;
        const uint8_t MHI_ECONO = 0x00;

        // Vertical swing
        const uint8_t MHI_VS_SWING = 0xE0;
        const uint8_t MHI_VS_UP = 0xC0;
        const uint8_t MHI_VS_MUP = 0xA0;
        const uint8_t MHI_VS_MIDDLE = 0x80;
        const uint8_t MHI_VS_MDOWN = 0x60;
        const uint8_t MHI_VS_DOWN = 0x40;
        const uint8_t MHI_VS_STOP = 0x20;

        // Horizontal swing
        const uint8_t MHI_HS_SWING = 0x0F;
        const uint8_t MHI_HS_MIDDLE = 0x0C;
        const uint8_t MHI_HS_LEFT = 0x0E;
        const uint8_t MHI_HS_MLEFT = 0x0D;
        const uint8_t MHI_HS_MRIGHT = 0x0B;
        const uint8_t MHI_HS_RIGHT = 0x0A;
        const uint8_t MHI_HS_STOP = 0x07;
        const uint8_t MHI_HS_LEFTRIGHT = 0x08;
        const uint8_t MHI_HS_RIGHTLEFT = 0x09;

        // Only available in Auto, Cool and Heat mode
        const uint8_t MHI_3DAUTO_ON = 0x00;
        const uint8_t MHI_3DAUTO_OFF = 0x12;

        // NOT available in Fan or Dry mode
        const uint8_t MHI_SILENT_ON = 0x00;
        const uint8_t MHI_SILENT_OFF = 0x80;

        // Pulse parameters in usec
        const uint16_t MHI_BIT_MARK = 400;
        const uint16_t MHI_ONE_SPACE = 1200;
        const uint16_t MHI_ZERO_SPACE = 400;
        const uint16_t MHI_HEADER_MARK = 3200;
        const uint16_t MHI_HEADER_SPACE = 1600;
        const uint16_t MHI_MIN_GAP = 17500;

        bool MhiClimate::on_receive(remote_base::RemoteReceiveData data) {
            ESP_LOGD(TAG, "Received some bytes");

            // The protocol sends the data twice, read here
            // uint32_t loop_read;
            
            uint8_t bytes[19] = {};

            //for (uint16_t loop = 1; loop <= 2; loop++) {
            if (!data.expect_item(MHI_HEADER_MARK, MHI_HEADER_SPACE))
                return false;
            
            // loop_read = 0;
            for (uint8_t a_byte = 0; a_byte < 19; a_byte++) {
                uint8_t byte = 0;
                for (int8_t a_bit = 0; a_bit < 8; a_bit++) {
                    if (data.expect_item(MHI_BIT_MARK, MHI_ONE_SPACE))
                        byte |= 1 << a_bit;
                    else if (!data.expect_item(MHI_BIT_MARK, MHI_ZERO_SPACE))
                        return false;
                }
                bytes[a_byte] = byte;
            }

            ESP_LOGD(TAG, 
                "Received bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            // Check the static bytes
            if (bytes[0] != 0x52 || bytes[1] != 0xAE || bytes[2] != 0xC3 || bytes[3] != 0x1A || bytes[4] != 0xE5) {
                return false;
            }

            ESP_LOGD(TAG, "Passed check 1");

            // Check the inversed bytes
            if (bytes[5] != (~bytes[6] & 0xFF)
                || bytes[7] != (~bytes[8] & 0xFF)
                || bytes[9] != (~bytes[10] & 0xFF)
                || bytes[11] != (~bytes[12] & 0xFF)
                || bytes[13] != (~bytes[14] & 0xFF)
                || bytes[15] != (~bytes[16] & 0xFF)
                || bytes[17] != (~bytes[18] & 0xFF)
            ) {
                return false;
            }
            
            ESP_LOGD(TAG, "Passed check 2");

            auto powerMode = bytes[5] & 0x08;
            auto operationMode = bytes[5] & 0x07;
            auto temperature = (~bytes[7] & 0x0F) + 17; 
            auto fanSpeed = bytes[9] & 0x0F;
            auto swingV = bytes[11] & 0xE0; // ignore the bit for the 3D auto
            auto swingH = bytes[13] & 0x0F;

            ESP_LOGD(TAG,
                "Resulting numbers: powerMode=0x%02X operationMode=0x%02X temperature=%d fanSpeed=0x%02X swingV=0x%02X swingH=0x%02X",
                powerMode, operationMode, temperature, fanSpeed, swingV, swingH
            );

            if (powerMode == MHI_ON) {
                // Power and operating mode
                switch (operationMode) {
                    case MHI_COOL:
                        this->mode = climate::CLIMATE_MODE_COOL;
                        //swingV = MHI_VS_UP;
                        break;
                    case MHI_HEAT:
                        this->mode = climate::CLIMATE_MODE_HEAT;
                        // swingV = MHI_VS_DOWN;
                        break;
                    case MHI_FAN:
                        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                        break;
                    case MHI_DRY:
                        this->mode = climate::CLIMATE_MODE_DRY;
                        break;
                    default:
                    case MHI_AUTO:
                        this->mode = climate::CLIMATE_MODE_AUTO;
                        // swingV = MHI_VS_MIDDLE;
                        break;
                }
            } else {
                this->mode = climate::CLIMATE_MODE_OFF;
            }

            // Temperature
            this->target_temperature = temperature;

            // Horizontal and vertical swing
            if (swingV == MHI_VS_SWING && swingH == MHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_BOTH;
            } else if (swingV == MHI_VS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
            } else if (swingH == MHI_HS_SWING) {
                this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
            } else {
                this->swing_mode = climate::CLIMATE_SWING_OFF;
            }

            // Fan speed
            switch (fanSpeed) {
                case MHI_FAN1:
                case MHI_FAN2: // Only to support remote feedback
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    break;
                case MHI_FAN3:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    break;
                case MHI_FAN4:
                case MHI_HIPOWER: // Not yet supported. Will be added when ESPHome supports it.
                    this->fan_mode = climate::CLIMATE_FAN_HIGH;
                    break;
                case MHI_FAN_AUTO:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    switch (swingH) {
                        case MHI_HS_MIDDLE:
                            this->fan_mode = climate::CLIMATE_FAN_MIDDLE;
                            break;
                        case MHI_HS_RIGHTLEFT:
                            this->fan_mode = climate::CLIMATE_FAN_FOCUS;
                            break;
                        case MHI_HS_LEFTRIGHT:
                            this->fan_mode = climate::CLIMATE_FAN_DIFFUSE;
                            break;
                    }
                case MHI_ECONO: // Not yet supported. Will be added when ESPHome supports it.
                default:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    break;
            }

            ESP_LOGD(TAG, "Finished da shizzel");
            
            this->publish_state();
            return true;

                // Useful for reverting status back to home assistant based on input:
                    // this->mode
                    // this->target_temperature
                    // this->swing_mode
                    // this->fan_mode
                    // climate::CLIMATE_MODE_OFF:
                    // climate::CLIMATE_MODE_AUTO:
                    // climate::CLIMATE_MODE_COOL:
                    // climate::CLIMATE_MODE_HEAT:
                    // climate::CLIMATE_MODE_FAN_ONLY:
                    // climate::CLIMATE_MODE_DRY:

                    // climate::CLIMATE_SWING_BOTH <= beide op auto
                    // climate::CLIMATE_SWING_HORIZONTAL <= auto
                    // climate::CLIMATE_SWING_VERTICAL <= auto
                    // climate::CLIMATE_SWING_OFF

                    // climate::CLIMATE_FAN_LOW
                    // climate::CLIMATE_FAN_MEDIUM
                    // climate::CLIMATE_FAN_HIGH
                    // climate::CLIMATE_FAN_MIDDLE
                    // climate::CLIMATE_FAN_FOCUS
                    // climate::CLIMATE_FAN_DIFFUSE
                    // climate::CLIMATE_FAN_AUTO


                    // enum ClimateMode : uint8_t {
                    //   /// The climate device is off (not in auto, heat or cool mode)
                    //   CLIMATE_MODE_OFF = 0,
                    //   /// The climate device is set to automatically change the heating/cooling cycle
                    //   CLIMATE_MODE_AUTO = 1,
                    //   /// The climate device is manually set to cool mode (not in auto mode!)
                    //   CLIMATE_MODE_COOL = 2,
                    //   /// The climate device is manually set to heat mode (not in auto mode!)
                    //   CLIMATE_MODE_HEAT = 3,
                    //   /// The climate device is manually set to fan only mode
                    //   CLIMATE_MODE_FAN_ONLY = 4,
                    //   /// The climate device is manually set to dry mode
                    //   CLIMATE_MODE_DRY = 5,
                    // };

                    // /// Enum for the current action of the climate device. Values match those of ClimateMode.
                    // enum ClimateAction : uint8_t {
                    //   /// The climate device is off (inactive or no power)
                    //   CLIMATE_ACTION_OFF = 0,
                    //   /// The climate device is actively cooling (usually in cool or auto mode)
                    //   CLIMATE_ACTION_COOLING = 2,
                    //   /// The climate device is actively heating (usually in heat or auto mode)
                    //   CLIMATE_ACTION_HEATING = 3,
                    //   /// The climate device is idle (monitoring climate but no action needed)
                    //   CLIMATE_ACTION_IDLE = 4,
                    //   /// The climate device is drying (either mode DRY or AUTO)
                    //   CLIMATE_ACTION_DRYING = 5,
                    //   /// The climate device is in fan only mode (either mode FAN_ONLY or AUTO)
                    //   CLIMATE_ACTION_FAN = 6,
                    // };

                    // /// Enum for all modes a climate fan can be in
                    // enum ClimateFanMode : uint8_t {
                    //   /// The fan mode is set to On
                    //   CLIMATE_FAN_ON = 0,
                    //   /// The fan mode is set to Off
                    //   CLIMATE_FAN_OFF = 1,
                    //   /// The fan mode is set to Auto
                    //   CLIMATE_FAN_AUTO = 2,
                    //   /// The fan mode is set to Low
                    //   CLIMATE_FAN_LOW = 3,
                    //   /// The fan mode is set to Medium
                    //   CLIMATE_FAN_MEDIUM = 4,
                    //   /// The fan mode is set to High
                    //   CLIMATE_FAN_HIGH = 5,
                    //   /// The fan mode is set to Middle
                    //   CLIMATE_FAN_MIDDLE = 6,
                    //   /// The fan mode is set to Focus
                    //   CLIMATE_FAN_FOCUS = 7,
                    //   /// The fan mode is set to Diffuse
                    //   CLIMATE_FAN_DIFFUSE = 8,
                    // };

                    // /// Enum for all modes a climate swing can be in
                    // enum ClimateSwingMode : uint8_t {
                    //   /// The sing mode is set to Off
                    //   CLIMATE_SWING_OFF = 0,
                    //   /// The fan mode is set to Both
                    //   CLIMATE_SWING_BOTH = 1,
                    //   /// The fan mode is set to Vertical
                    //   CLIMATE_SWING_VERTICAL = 2,
                    //   /// The fan mode is set to Horizontal
                    //   CLIMATE_SWING_HORIZONTAL = 3,
                    // };

                    // /// Convert the given



                    // // Need to see this segment inverted
                    // for (int8_t a_bit = 7; a_bit >= 0; a_bit--) {
                        // bool bit = byte & (1 << a_bit);
                        // if (!data.expect_item(BIT_MARK_US, bit ? BIT_ZERO_SPACE_US : BIT_ONE_SPACE_US))
                            // return false;
                    // }
                    // // // Receiving MSB first: reorder bytes
                    // // loop_read |= byte << ((2 - a_byte) * 8);
                // }
                
                // // Footer Mark
                // if (!data.expect_mark(BIT_MARK_US))
                    // return false;
                
                // // if (loop == 1) {
                    // // // Back up state on first loop
                    // // remote_state = loop_read;
                    // // if (!data.expect_space(FOOTER_SPACE_US))
                        // // return false;
                // // }
            //}
        }

        void MhiClimate::transmit_state() {
            uint8_t remote_state[] = {
                0x52, 0xAE, 0xC3, 0x1A,
                0xE5, 0x90, 0x00, 0xF0,
                0x00, 0xF0, 0x00, 0x0D,
                0x00, 0x10, 0x00, 0xFF,
                0x00, 0x7F, 0x00
            };

            // ----------------------
            // Initial values
            // ----------------------

            auto operatingMode = MHI_AUTO;
            auto powerMode = MHI_ON;
            auto cleanMode = 0x60; // always off

            auto temperature = 22;
            auto fanSpeed = MHI_FAN_AUTO;
            auto swingV = MHI_VS_STOP;
            auto swingH = MHI_HS_RIGHT;  // custom preferred value for this mode, should be MHI_HS_STOP
            auto _3DAuto = MHI_3DAUTO_OFF;
            auto silentMode = MHI_SILENT_OFF;

            // ----------------------
            // Assign the values
            // ----------------------

            // Power and operating mode
            switch (this->mode) {
                case climate::CLIMATE_MODE_COOL:
                    operatingMode = MHI_COOL;
                    swingV = MHI_VS_UP; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_HEAT:
                    operatingMode = MHI_HEAT;
                    swingV = MHI_VS_DOWN; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_AUTO:
                    operatingMode = MHI_AUTO;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    operatingMode = MHI_FAN;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_DRY:
                    operatingMode = MHI_DRY;
                    swingV = MHI_VS_MIDDLE; // custom preferred value for this mode
                    break;
                case climate::CLIMATE_MODE_OFF:
                default:
                    powerMode = MHI_OFF;
                    break;
            }

            // Temperature
            if (this->target_temperature > 17 && this->target_temperature < 31)
                temperature = this->target_temperature;

            // Horizontal and vertical swing
            switch (this->swing_mode) {
                case climate::CLIMATE_SWING_BOTH:
                    swingV = MHI_VS_SWING;
                    swingH = MHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_HORIZONTAL:
                    swingH = MHI_HS_SWING;
                    break;
                case climate::CLIMATE_SWING_VERTICAL:
                    swingV = MHI_VS_SWING;
                    break;
                case climate::CLIMATE_SWING_OFF:
                default:
                    // Already on STOP
                    break;
            }

            // Fan speed
            switch (this->fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    fanSpeed = MHI_FAN1;
                    break;
                case climate::CLIMATE_FAN_MEDIUM:
                    fanSpeed = MHI_FAN3;
                    break;
                case climate::CLIMATE_FAN_HIGH:
                    fanSpeed = MHI_FAN4;
                    break;
                case climate::CLIMATE_FAN_MIDDLE:
                    fanSpeed = MHI_FAN_AUTO;
                    swingH = MHI_HS_MIDDLE;
                    break;
                case climate::CLIMATE_FAN_FOCUS:
                    fanSpeed = MHI_FAN_AUTO;
                    swingH = MHI_HS_RIGHTLEFT;
                    break;
                case climate::CLIMATE_FAN_DIFFUSE:
                    fanSpeed = MHI_FAN_AUTO;
                    swingH = MHI_HS_LEFTRIGHT;
                    break;
                case climate::CLIMATE_FAN_AUTO:
                default:
                    fanSpeed = MHI_FAN_AUTO;
                    break;
            }

            // ----------------------
            // Assign the bytes
            // ----------------------

            // Power state + operating mode
            remote_state[5] |= powerMode | operatingMode | cleanMode;

            // Temperature
            remote_state[7] |= (~((uint8_t)temperature - 17) & 0x0F);

            // Fan speed
            remote_state[9] |= fanSpeed;

            // Vertical air flow + 3D auto
            remote_state[11] |= swingV | _3DAuto;

            // Horizontal air flow
            remote_state[13] |= swingV | swingH;

            // Silent
            remote_state[15] |= silentMode;

            // There is no checksum, but some bytes are inverted
            remote_state[6] = ~remote_state[5];
            remote_state[8] = ~remote_state[7];
            remote_state[10] = ~remote_state[9];
            remote_state[12] = ~remote_state[11];
            remote_state[14] = ~remote_state[13];
            remote_state[16] = ~remote_state[15];
            remote_state[18] = ~remote_state[17];

            // ESP_LOGD(TAG, "Sending MHI target temp: %.1f state: %02X mode: %02X temp: %02X", this->target_temperature, remote_state[5], remote_state[6], remote_state[7]);

            auto bytes = remote_state;
            ESP_LOGD(TAG, 
                "Sent bytes 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5], bytes[6], bytes[7],
                bytes[8], bytes[9], bytes[10], bytes[11],
                bytes[12], bytes[13], bytes[14], bytes[15],
                bytes[16], bytes[17], bytes[18]
            );

            auto transmit = this->transmitter_->transmit();
            auto data = transmit.get_data();

            data->set_carrier_frequency(38000);

            // Header
            data->mark(MHI_HEADER_MARK);
            data->space(MHI_HEADER_SPACE);

            // Data
            for (uint8_t i : remote_state)
                for (uint8_t j = 0; j < 8; j++) {
                    data->mark(MHI_BIT_MARK);
                    bool bit = i & (1 << j);
                    data->space(bit ? MHI_ONE_SPACE : MHI_ZERO_SPACE);
                }
            data->mark(MHI_BIT_MARK);
            data->space(0);

            transmit.perform();
        }
    } // namespace mhi
} // namespace esphome
