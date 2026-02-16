#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

// Using PROGMEM to save values on flash memory instead of ram
const uint32_t raw_red_data[] PROGMEM = {
  116394, 116496, 116534, 116440, 115964, 115684, 115678, 115754, 115801, 115830,
  115763, 115684, 115670, 115710, 115786, 115865, 115959, 116034, 116102, 116169,
  116232, 116305, 116394, 116427, 116009, 115582, 115476, 115558, 115630, 115692,
  115690, 115586, 115541, 115550, 115653, 115701, 115817, 115864, 115979, 116001,
  116127, 116153, 116274, 116342, 116369, 115851, 115431, 115301, 115428, 115483
};

const uint32_t raw_ir_data[] PROGMEM = {
  105863, 105901, 105945, 105795, 105612, 105566, 105598, 105629, 105655, 105665,
  105631, 105606, 105619, 105665, 105704, 105736, 105768, 105797, 105846, 105856,
  105899, 105938, 105983, 105899, 105711, 105576, 105605, 105637, 105679, 105685,
  105676, 105639, 105634, 105661, 105702, 105744, 105770, 105796, 105831, 105869,
  105903, 105944, 105989, 106042, 105925, 105711, 105602, 105636, 105665, 105688
};

class Sensor {
public:
    Sensor() {}
    ~Sensor() {}

    void setup(byte ledBrightness, byte  sampleAverage, byte  ledMode, int sampleRate, int pulseWidth, int adcRange) {}

    uint32_t getRed() {
        uint32_t val = pgm_read_dword(&(raw_red_data[pos]));
        return applyNoise(val, false);
    }

    uint32_t getIR() {
        uint32_t val = pgm_read_dword(&(raw_ir_data[pos]));
        uint32_t noisyVal = applyNoise(val, true);

        // To have a continous heartbeat we loop forward and then backwards.
        if (going_up) {
            pos++;
            if (pos >= 49) {
                going_up = false;
            }
        }
        else {
            pos--;
            if (pos == 0) {
                going_up = true;
            }
        }

        return noisyVal;
    }

private:
    uint32_t applyNoise(uint32_t clean_value, bool is_ir) {
        int32_t value = (int32_t)clean_value;

        // Electronic Noise
        int electronic_noise = random(-5, 6);
        value += electronic_noise;

        if (is_ir) {
            // Baseline Wander
            drift = sin(breath_phase) * 30.0;
            breath_phase += 0.05;

            // Motion Artifact Trigger
            if (random(0, 300) == 5) {
                motion_offset = random(-500, 500);
            }

            // Decay
            motion_offset *= 0.92;
            if (abs(motion_offset) < 1) motion_offset = 0;
        }

        if (is_ir) value += (int32_t)drift;
        else       value += (int32_t)(drift * 0.2);

        value += (int32_t)motion_offset;

        if (value < 0) value = 0;

        return (uint32_t)value;
    }

private:
    float motion_offset = 0;
    float drift = 0;
    uint8_t pos = 0;
    float breath_phase = 0;

    bool going_up = true;
};