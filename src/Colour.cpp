#include <Arduino.h>
#include <Colour.h>

uint16_t RGBLed::FLASH_SLOW = 400;
uint16_t RGBLed::FLASH_FAST = 200;

RGBLed::RGBLed(int rPin, int gPin, int bPin) {
  this->rPin = rPin;
  this->gPin = gPin;
  this->bPin = bPin;

  startMillis = 0;
  flashOn = false;
  flashDuration = FLASH_FAST;
  remainOnAfterAnimation = false;
  defaultColour = 0x000000;

  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);

  off();
}

void RGBLed::setDefaultColour(uint32_t colour) {
  this->defaultColour = colour;
  write(red(colour), green(colour), blue(colour));
}

void RGBLed::colour(uint8_t r, uint8_t g, uint8_t b, uint16_t duration) {
  colour(((uint32_t)r << 16) | (g << 8) | b, duration);
}

void RGBLed::colour(uint32_t rgb) {
  this->sourceColour = rgb;
  this->destColour = rgb;
  this->startMillis = millis();
  this->duration = 0;
  this->remainOnAfterAnimation = true;

  write(red(rgb), green(rgb), blue(rgb));
}

void RGBLed::colour(uint32_t rgb, uint16_t duration) {
  this->sourceColour = rgb;
  this->destColour = rgb;
  this->startMillis = millis();
  this->duration = duration;
  this->remainOnAfterAnimation = false;

  write(red(rgb), green(rgb), blue(rgb));
}

void RGBLed::flash(uint32_t colour, uint8_t numFlashes, uint16_t speed) {
  this->sourceColour = colour;
  this->destColour = colour;
  this->startMillis = millis();
  this->flashDuration = speed;
  this->duration = numFlashes * speed * 2 - speed;
  this->remainOnAfterAnimation = false;
  this->numFlashes = numFlashes;
}

void RGBLed::off() {
  colour(0x000000, 0);
}

void RGBLed::interpolateTo(uint8_t sourceR, uint8_t sourceG, uint8_t sourceB, uint8_t destR, uint8_t destG, uint8_t destB, uint16_t duration, bool remainOnAfterAnimation) {
  this->interpolateTo(
    ((uint32_t)sourceR << 16) | (sourceG << 8) | sourceB,
    ((uint32_t)destR << 16) | (destG << 8) | destB,
    duration,
    remainOnAfterAnimation
  );
}

void RGBLed::interpolateTo(uint32_t sourceColour, uint32_t destColour, uint16_t duration, bool remainOnAfterAnimation) {
  this->sourceColour = sourceColour;
  this->destColour = destColour;
  this->startMillis = millis();
  this->duration = duration;
  this->remainOnAfterAnimation = remainOnAfterAnimation;
}

void RGBLed::write(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(rPin, r / RGBLed_DAMPEN);
    analogWrite(gPin, g / RGBLed_DAMPEN);
    analogWrite(bPin, b / RGBLed_DAMPEN);
}

void RGBLed::process() {

  if (startMillis == 0) {
    return;
  }

  unsigned long time = millis();
  long remainingMillis = startMillis + duration - time;
  if (remainingMillis <= 0) {

    if (!remainOnAfterAnimation) {

      write(red(defaultColour), green(defaultColour), blue(defaultColour));

    } else if (sourceColour != destColour) {

      // The last call to process() prior to elapsing our full duration probably left
      // the LED somewhere close-to-but-not-quite-100% of the way to destColour, so
      // explicitly set it to dest colour before clearing our state and returning.
      write(red(destColour), green(destColour), blue(destColour));

    }

    remainOnAfterAnimation = false;
    startMillis = 0;
    flashOn = false;
    return;
  }

  if (numFlashes > 0) {

    bool shouldFlashBeOn = ((time - startMillis) / flashDuration) % 2 == 0;
    if (shouldFlashBeOn && !flashOn) {
      flashOn = true;
      write(red(sourceColour), green(sourceColour), blue(destColour));
    } else if (!shouldFlashBeOn && flashOn) {
      flashOn = false;
      write(0, 0, 0);
    }

  } else if (sourceColour != destColour) {
    // No need to continually write the same colour - it was alerady done once when we
    // first asked to show these colours.

    float factor = 1 - (float)remainingMillis / duration;
    
    uint8_t sourceR = red(sourceColour);
    uint8_t sourceG = green(sourceColour);
    uint8_t sourceB = blue(sourceColour);

    uint8_t destR = red(destColour);
    uint8_t destG = green(destColour);
    uint8_t destB = blue(destColour);

    uint8_t r = (sourceR + (destR - sourceR) * factor);
    uint8_t g = (sourceG + (destG - sourceG) * factor);
    uint8_t b = (sourceB + (destB - sourceB) * factor);
    
    write(r, g, b);
  }
}