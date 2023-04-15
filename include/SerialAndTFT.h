//file: SerialAndTFT.h

#ifndef SERIALANDTFT_H
#define SERIALANDTFT_H

#include <Print.h>
#include <TFT_eSPI.h>

class SerialAndTFT : public Print {
public:
  SerialAndTFT(TFT_eSPI &tft) : _tft(tft) {}

  size_t write(uint8_t ch) override {
    Serial.write(ch);
    _tft.write(ch);
    return 1;
  }

private:
  TFT_eSPI &_tft;
};

#endif // SERIALANDTFT_H
