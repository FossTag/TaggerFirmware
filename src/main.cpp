#include <Arduino.h>

// #define SEND_PWM_BY_TIMER

/**
 * Required.
 * Used to differentiate each device, so that we can ignore IR data from our own transmitter.
 * TODO: Make this externalised via config, or more practically via a special controller that transmits commands to each device.
 */
#define MY_ADDRESS 1

/**
 * Optional (but highlighly recommended or else your device cannot fire).
 * Connected to a momentary switch, where one side is grounded and the other side is this pin.
 * Will be connected to the device via an internal pullup resistor, so no additional resistors are required.
 */
#define FIRE_BUTTON_PIN 6

/**
 * Optional (but highly recommended or else your device cannot fire).
 * Emits IR signals at 38Khz using the Sony protocol each time FIRE_BUTTON_PIN is triggered.
 */
#define FIRE_IR_SEND_PIN 9

/**
 * Required if FIRE_BUTTON_PIN is set, otherwise not required.
 * Minimum time allowed between triggering fires.
 * If the fire button is held down, then it will send signals every this often.
 */
#define FIRE_COOLOFF_MILLIS 500

/**
 * Optional (but highly recommended or else your device cannot be hit and will be indistructable).
 * Receives IR signals at 38Khz using the Sony protocol and interprets them as hit or configuration messages.
 * Can wire multiple IR Receivers in parallel all to this pin.
 * Recommended to include a capacitor in the circuit (e.g. 10uF) as per https://web.archive.org/web/20161210114703/http://www.lasertagparts.com/mtsensors.htm.
 */
#define HIT_IR_RECEIVE_PIN 8

/**
 * Optional.
 * Flashes for STATUS_FIRE_LED_DURATION_MILLIS each time FIRE_BUTTON_PIN is triggered.
 * Flashes for STATUS_HIT_LED_DURATION_MILLIS each time player is hit.
 * TODO: Defaults to the colour of the players team.
 */
#define STATUS_LED_R_PIN 3
#define STATUS_LED_G_PIN 10
#define STATUS_LED_B_PIN 11


#define SPEAKER_PIN 7

#define DECODE_SONY
#define NO_LED_FEEDBACK_CODE

#ifdef SPEAKER_PIN
/**
 * When using the SPEAKER_PIN for feedback, the Arduino tone() will use Timer 2 which is the default used for many boards by the IRremote library.
 * https://github.com/Arduino-IRremote/Arduino-IRremote#incompatibilities-to-other-libraries-and-arduino-commands-like-tone-and-analogwrite
 * TODO: To support more boards with arduino-laser-tag, we will need to carefully choose timers here as per the docs above.
 */
#define IR_USE_AVR_TIMER1
#endif

#if defined STATUS_LED_R_PIN && defined STATUS_LED_G_PIN && defined STATUS_LED_B_PIN
#define HAS_STATUS_LED
#endif

#include <IRremote.hpp>
#include <Sound.h>
#include <Colour.h>

/**
 * Used for both debouncing fires, limiting the rate of fire, and also used to only
 * show feedback of firing for a short time.
 */
long lastFireTime = 0L;

/**
 * You can get hit lots of times, but this will debounce briefly to ignore the effects
 * of reflected IR, as well as multiple IR receivers wired together from catching the
 * same burst of IR multiple times.
 * It is also used to only show feedback of being hit for a short time.
 */
long lastHitTime = 0L;

int health = 1000;

void printConfig() {
  Serial.println("arduino-laser-tag has the following pin configurations:");

  Serial.println("");
  Serial.println("HIT_IR_RECEIVE_PIN");
  #ifdef HIT_IR_RECEIVE_PIN
  Serial.print("  Pin: ");
  Serial.println(HIT_IR_RECEIVE_PIN);
  Serial.println("  Wiring: 38KHz IR receiver (e.g. TSOP38238).");
  Serial.println("          Can be multiple receivers wired in series (e.g. a few mounted on a helmet, shoulders, gun, etc).");
  #else
  Serial.println("N/A (Unable to receive hits from other players. May be useful for things like umpires devices, but that is about all.)");
  #endif

  Serial.println("");
  Serial.println("FIRE_IR_SEND_PIN");
  #ifdef FIRE_IR_SEND_PIN
  Serial.print("  Pin: ");
  Serial.println(FIRE_IR_SEND_PIN);
  Serial.println("  Wiring: IR LED with the anode connected to this pin.");
  Serial.println("          Get maximum range by using a high powered LED (e.g. 100mA) with a narrow viewing angle and an appropriate lens mounted in front.");
  Serial.println("          If you require more power than this pin can deliver, connect a transitor to this pin instead to drive the LED.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Unable to fire at other players.");
  Serial.println("  May be useful for, e.g. static firing range targets, but not much else.");
  #endif
  
  Serial.println("");
  Serial.println("FIRE_BUTTON_PIN");
  #ifdef FIRE_BUTTON_PIN
  Serial.print("  Pin: ");
  Serial.println(FIRE_BUTTON_PIN);
  Serial.println("  Wiring: Momentary switch with one side wired to ground, the other here.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Will not be able to fire.");
  Serial.println("  May be desirable for, e.g. static firing range targets, but not much else.");
  #endif

  Serial.println("");
  Serial.println("FIRE_STATUS_LED_[R, G, B]_PIN");
  #ifdef HAS_STATUS_LED
  Serial.print("  Pins (R, G, B): ");
  Serial.print(STATUS_LED_R_PIN);
  Serial.print(", ");
  Serial.print(STATUS_LED_R_PIN);
  Serial.print(", ");
  Serial.println(STATUS_LED_R_PIN);
  Serial.println("  Wiring: Connect to a common cathode RGB LED.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Will not show visual feedback when getting hit or firing, and will not be able to show team colours.");
  #endif

  Serial.println("");
  Serial.println(" SPEAKER_PIN");
  #ifdef SPEAKER_PIN
  Serial.print("  Pin: ");
  Serial.println(SPEAKER_PIN);
  Serial.println("  Wiring: Ensure this pin has enough power to drive the speaker.");
  Serial.println("          If not, connect a transistor to this pin instead of directly connecting the speaker.");
  Serial.println("          Also wire a snubber diode if required.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Will not provide any audio feedback when firing, getting hit, dying, configuring, etc.");
  #endif
  
  Serial.println("");
}

#ifdef HAS_STATUS_LED
RGBLed statusLed = RGBLed(STATUS_LED_R_PIN, STATUS_LED_G_PIN, STATUS_LED_B_PIN);
#endif

void setup() {
  Serial.begin(115200);
  printConfig();

  #ifdef HAS_STATUS_LED
  statusLed.setDefaultColour(0x00FF00);
  #endif

  #ifdef FIRE_BUTTON_PIN
  pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
  #endif

  #ifdef HIT_IR_RECEIVE_PIN
  IrReceiver.begin(HIT_IR_RECEIVE_PIN, true);
  #endif

  #ifdef FIRE_IR_SEND_PIN
  IrSender.begin(FIRE_IR_SEND_PIN, false, 0);
  #endif
}

#ifdef HIT_IR_RECEIVE_PIN
bool wasHit() {
  bool hit = false;
  if (IrReceiver.decode()) {
    Serial.print(F("Decoded protocol: "));
    Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
    Serial.print(F(", Decoded raw data: "));
    Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
    Serial.print(F(", decoded address: "));
    Serial.print(IrReceiver.decodedIRData.address, HEX);
    Serial.print(F(", decoded command: "));
    Serial.println(IrReceiver.decodedIRData.command, HEX);

    if (IrReceiver.decodedIRData.decodedRawData == 0) {
      Serial.println("Ignoring hit becaues it doesn't seem to be a full message received.");
    } else if (IrReceiver.decodedIRData.address == MY_ADDRESS) {
      Serial.println("Ignoring hit because it was from my IR LED.");
    } else {
      hit = true;
    }
    IrReceiver.resume();
  }

  return hit;
}
#endif

#ifdef SPEAKER_PIN
RandomDescendingNoteSound hitSound = RandomDescendingNoteSound(
  30, 60,
  10,
  150, 50, 50
);

RandomDescendingNoteSound fireSound = RandomDescendingNoteSound(
  30, 60,
  5,
  300, 240, 50
);

RandomDescendingNoteSound dieSound = RandomDescendingNoteSound(
  50, 100,
  30,
  360, 0, 120
);
#endif

void loop() {

  #ifdef FIRE_BUTTON_PIN
  if (digitalRead(FIRE_BUTTON_PIN) == LOW && millis() - lastFireTime > FIRE_COOLOFF_MILLIS) {

    Serial.println("Fire button pressed");
    lastFireTime = millis();

    #ifdef FIRE_IR_SEND_PIN
    IrSender.sendSony(MY_ADDRESS, 0x1, 0);
    #endif

    #ifdef HAS_STATUS_LED
    statusLed.flash(0xFFFFFF, 1, RGBLed::FLASH_FAST);
    #endif

    #ifdef SPEAKER_PIN
    Sound::play(&fireSound);
    #endif

  }
  #endif

  #ifdef HIT_IR_RECEIVE_PIN
  if (wasHit()) {
    Serial.println("Hit!");
    lastHitTime = millis();

    health -= 30;

    #ifdef HAS_STATUS_LED
    statusLed.flash(0xFF0000, 1, RGBLed::FLASH_FAST);
    #endif

    #ifdef SPEAKER_PIN
    if (health < 0) {
      Sound::play(&dieSound);
    } else {
      Sound::play(&hitSound);
    }
    #endif
  }
  #endif

  #ifdef SPEAKER_PIN
  Sound::process(SPEAKER_PIN);
  #endif
  
  #ifdef HAS_STATUS_LED
  statusLed.process();
  #endif
  
}