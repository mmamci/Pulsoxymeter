#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "sensor.h"

Sensor sensor;

constexpr int32_t buffer_length = 100;
uint16_t irBuffer[buffer_length]{};
uint16_t redBuffer[buffer_length]{};

constexpr uint8_t moving_average_size = 4;
uint16_t irBufferHistory[moving_average_size]{};
uint16_t redBufferHistory[moving_average_size]{};

int32_t spo2;
int8_t validSPO2;
int32_t bpm;
int8_t validHeartRate;

float x_prev_red = 0;
float y_prev_red = 0;
float x_prev_ir = 0;
float y_prev_ir = 0;

const float R = 0.95;

int16_t dc_blocker(uint32_t x_current, float& x_prev, float& y_prev) {
  if (x_prev == 0) {
    x_prev = (float)x_current;
  }

  float y_current = (float)x_current - x_prev + (R * y_prev);

  x_prev = (float)x_current;
  y_prev = y_current;

  return (int16_t)y_current;
}

void apply_moving_average(uint16_t* buffer, uint16_t* buffer_history) {
  int32_t sum = 0;
  for (int i = 0; i < buffer_length; i++) {
    sum = 0;
    for (int k = 0; k < moving_average_size; k++) {
      int idx = i - k;
      if (idx >= 0) sum += buffer[idx];
      else sum += buffer_history[moving_average_size + idx];
    }
    buffer[i] = (uint16_t)(sum / moving_average_size);
  }
}

void setup() {
  Serial.begin(115200);
  // Sensor Simulation Setup
  sensor.setup(60, 4, 2, 100, 411, 4096);
}

void loop() {
  for (byte i = 0; i < buffer_length; i++) {
    uint32_t raw_red = sensor.getRed();
    uint32_t raw_ir = sensor.getIR();

    int16_t ac_red = dc_blocker(raw_red, x_prev_red, y_prev_red);
    int16_t ac_ir = dc_blocker(raw_ir, x_prev_ir, y_prev_ir);

    // Inverting signal for correct peaks and amplifying ir signal
    int32_t processed_red = -ac_red * 1;
    int32_t processed_ir = -ac_ir * 4;

    // Shifting values to vary around 30000 
    int32_t buffer_val_red = processed_red + 30000;
    int32_t buffer_val_ir = processed_ir + 30000;

    // Clamping
    if (buffer_val_red < 0) buffer_val_red = 0;
    if (buffer_val_ir < 0) buffer_val_ir = 0;
    if (buffer_val_red > 65535) buffer_val_red = 65535;
    if (buffer_val_ir > 65535) buffer_val_ir = 65535;

    redBuffer[i] = (uint16_t)buffer_val_red;
    irBuffer[i] = (uint16_t)buffer_val_ir;

    Serial.print(buffer_val_red); Serial.print(",");
    Serial.print(buffer_val_ir); Serial.print(",");
    Serial.print(spo2); Serial.print(",");
    Serial.print(bpm); Serial.print(",");
    Serial.println(millis());

    Serial.print(">offset_red:");
    Serial.println(buffer_val_red);

    Serial.print(">offset_ir:");
    Serial.println(buffer_val_ir);

    delay(40); // Simulating 25Hz
  }

  for (byte i = 0; i < moving_average_size; i++) {
    irBufferHistory[i] = irBuffer[buffer_length - moving_average_size + i];
    redBufferHistory[i] = redBuffer[buffer_length - moving_average_size + i];
  }

  apply_moving_average(irBuffer, irBufferHistory);
  apply_moving_average(redBuffer, redBufferHistory);

  maxim_heart_rate_and_oxygen_saturation(irBuffer, buffer_length, redBuffer, &spo2, &validSPO2, &bpm, &validHeartRate);
}