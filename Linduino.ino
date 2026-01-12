#include <Arduino.h>
#include <SPI.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "LTC2668.h"

// === Configuration ===
#define REF_INTERNAL 0
#define LTC2668_SPAN_BIPOLAR_5V       0x0002
#define LTC2668_SPAN_BIPOLAR_10V      0x0003

// === Simple EEPROM-free span table ===
uint8_t soft_span_range[16];

// ------------------ Helpers ------------------

String readLine() {
  while (!Serial.available());
  String s = Serial.readStringUntil('\n');
  s.trim();
  return s;
}

uint16_t voltageToCode(float v, uint8_t ch) {
  return LTC2668_voltage_to_code(
    v,
    LTC2668_MIN_OUTPUT[soft_span_range[ch]],
    LTC2668_MAX_OUTPUT[soft_span_range[ch]]
  );
}

void setAllChannelsToZero()
{
  for (uint8_t ch = 0; ch < 16; ch++) {
    uint16_t code = voltageToCode(0.0, ch);  // convert 0 V to DAC code
    LTC2668_write(LTC2668_CS, LTC2668_CMD_WRITE_N, ch, code);
  }

  // Apply all writes at once
  LTC2668_write(LTC2668_CS, LTC2668_CMD_UPDATE_ALL, 0, 0);

  Serial.println(F("All channels set to 0.000 V"));
}


void initDAC() 
{
  // 1) Enable internal reference (0 = internal enabled for this driver)
  LTC2668_write(LTC2668_CS, LTC2668_CMD_CONFIG, 0, REF_INTERNAL);

  // 2) Set all channels to ±5 V span (use the correct enum/define for your header)
  // Common names in LTC2668.h are like LTC2668_SPAN_±5V or LTC2668_SPAN_BIPOLAR_5V
  const uint16_t SPAN_BIPOLAR_5V = LTC2668_SPAN_BIPOLAR_5V;  // adjust if your header uses a different symbol

  for (uint8_t ch = 0; ch < 16; ch++) {
    LTC2668_write(LTC2668_CS, LTC2668_CMD_SPAN, ch, SPAN_BIPOLAR_5V);
    soft_span_range[ch] = (uint8_t)SPAN_BIPOLAR_5V;
  }

  // 3) Push settings and update
  LTC2668_write(LTC2668_CS, LTC2668_CMD_UPDATE_ALL, 0, 0);

  // 4) FORCE ALL OUTPUTS TO 0 V
  setAllChannelsToZero();

  Serial.println(F("LTC2668 set to internal ref and bipolar 5 V span on all channels."));
}

// ------------------ Arduino ------------------

void setup() {
  quikeval_SPI_init();
  quikeval_SPI_connect();

  Serial.begin(115200);
  delay(500);

  initDAC();
}

void loop() {
  if (!Serial.available()) return;

  String line = readLine();        // e.g. "SET 0 1"
  line.trim();

  // Find tokens separated by spaces
  int sp1 = line.indexOf(' ');
  if (sp1 < 0) { Serial.println("ERR CMD"); return; }

  String cmd = line.substring(0, sp1);
  cmd.toUpperCase();

  int sp2 = line.indexOf(' ', sp1 + 1);
  if (sp2 < 0) { Serial.println("ERR CMD"); return; }

  String chTok = line.substring(sp1 + 1, sp2);
  String vTok  = line.substring(sp2 + 1);
  chTok.trim();
  vTok.trim();

  if (cmd != "SET") { Serial.println("ERR CMD"); return; }

  int ch = chTok.toInt();
  float v = vTok.toFloat();
  

  // extra safety: reject empty / non-numeric voltage tokens like "abc"
  if (vTok.length() == 0) { Serial.println("ERR CMD"); return; }

  if (ch < 0 || ch > 15) { Serial.println("ERR BAD_CHANNEL"); return; }
  if (v < -5.0 || v > 5.0) { Serial.println("ERR RANGE"); return; }

  uint16_t code = voltageToCode(-v, (uint8_t)ch);
  LTC2668_write(LTC2668_CS, LTC2668_CMD_WRITE_N_UPDATE_N, (uint8_t)ch, code);

  Serial.print("OK CH ");
  Serial.print(ch);
  Serial.print(" = ");
  Serial.println(v, 4);
}
