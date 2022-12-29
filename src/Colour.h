#ifndef ARDUINO_LASER_TAG_LIGHT_H
#define ARDUINO_LASER_TAG_LIGHT_H

#define red(rgb) (rgb & 0xFF0000) >> 16
#define green(rgb) (rgb & 0x00FF00) >> 8
#define blue(rgb) rgb & 0x0000FF

#define RGBLed_DAMPEN 1

class RGBLed {
  public:
    RGBLed(int rPin, int gPin, int bPin);
    ~RGBLed() {};
    void off();
    void flash(uint32_t colour, uint8_t numFlashes, uint16_t speed);
    void colour(uint32_t colour);
    void colour(uint32_t colour, uint16_t duration);
    void colour(uint8_t r, uint8_t g, uint8_t b, uint16_t duration);
    void interpolateTo(uint8_t sourceR, uint8_t sourceG, uint8_t sourceB, uint8_t destR, uint8_t destG, uint8_t destB, uint16_t duration, bool remainOnAfterAnimation = true);
    void interpolateTo(uint32_t sourceColour, uint32_t destColour, uint16_t duration, bool remainOnAfterAnimation = true);
    void setDefaultColour(uint32_t colour);
    void process();

  public:
    static uint16_t FLASH_SLOW;
    static uint16_t FLASH_FAST;

  private:
    int rPin, gPin, bPin;
    uint32_t sourceColour, destColour, defaultColour;
    unsigned long startMillis;
    uint16_t duration;
    bool remainOnAfterAnimation;
    uint8_t numFlashes;
    uint16_t flashDuration;
    bool flashOn;


  private:
    void write(uint8_t r, uint8_t g, uint8_t b);
};

#endif