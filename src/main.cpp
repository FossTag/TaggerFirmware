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
#define FIRE_BUTTON_PIN 7

/**
 * Optional (but highly recommended or else your device cannot fire).
 * Emits IR signals at 38Khz using the Sony protocol each time FIRE_BUTTON_PIN is triggered.
 */
#define FIRE_IR_SEND_PIN 6

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
#define HIT_IR_RECEIVE_PIN 9

/**
 * Optional.
 * Flashes for FIRE_STATUS_LED_DURATION_MILLIS each time FIRE_BUTTON_PIN is triggered.
 * Defauts to LOW when off, and set to HIGH to turn LED on.
 */
#define FIRE_STATUS_LED_PIN 5
#define FIRE_STATUS_LED_DURATION_MILLIS 100

/**
 * Optional.
 * Flashes for HIT_STATUS_LED_DURATION_MILLIS each time the device registers a hit on HIT_IR_RECEIVE_PIN.
 * Defauts to LOW when off, and set to HIGH to turn LED on.
 */
#define HIT_STATUS_LED_PIN 4
#define HIT_STATUS_LED_DURATION_MILLIS 200

#define SPEAKER_PIN 8

#define DECODE_SONY

#ifdef SPEAKER_PIN
/**
 * When using the SPEAKER_PIN for feedback, the Arduino tone() will use Timer 2 which is the default used for many boards by the IRremote library.
 * https://github.com/Arduino-IRremote/Arduino-IRremote#incompatibilities-to-other-libraries-and-arduino-commands-like-tone-and-analogwrite
 * TODO: To support more boards with arduino-laser-tag, we will need to carefully choose timers here as per the docs above.
 */
#define IR_USE_AVR_TIMER1
#endif

#include <IRremote.hpp>
#include <Sound.h>

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
  Serial.println("FIRE_STATUS_LED_PIN");
  #ifdef FIRE_STATUS_LED_PIN
  Serial.print("  Pin: ");
  Serial.println(FIRE_STATUS_LED_PIN);
  Serial.println("  Wiring: Cathode wired to this pin, other to ground.");
  Serial.print("  Config: Will turn on for ");
  Serial.print(FIRE_STATUS_LED_DURATION_MILLIS);
  Serial.println("ms after firing.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Will not show visual feedback when getting hit.");
  #endif

  Serial.println("");
  Serial.println("HIT_STATUS_LED_PIN");
  #ifdef HIT_STATUS_LED_PIN
  Serial.print("  Pin: ");
  Serial.println(HIT_STATUS_LED_PIN);
  Serial.println("  Wiring: Cathode wired to this pin, other to ground.");
  Serial.print("  Config: Will turn on for ");
  Serial.print(HIT_STATUS_LED_DURATION_MILLIS);
  Serial.println("ms after being hit by another player.");
  #else
  Serial.println("  Not configured.");
  Serial.println("  Will not show visual feedback when getting hit.");
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

void setup() {
  Serial.begin(115200);
  printConfig();

  #ifdef FIRE_BUTTON_PIN
  pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
  #endif

  #ifdef FIRE_STATUS_LED_PIN
  pinMode(FIRE_STATUS_LED_PIN, OUTPUT);
  digitalWrite(FIRE_STATUS_LED_PIN, LOW);
  #endif

  #ifdef HIT_STATUS_LED_PIN
  pinMode(HIT_STATUS_LED_PIN, OUTPUT);
  digitalWrite(HIT_STATUS_LED_PIN, LOW);
  #endif

  #ifdef HIT_IR_RECEIVE_PIN
  IrReceiver.begin(HIT_IR_RECEIVE_PIN, true);
  #endif

  #ifdef FIRE_IR_SEND_PIN
  IrSender.begin(FIRE_IR_SEND_PIN, true, LED_BUILTIN);
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

    #ifdef FIRE_STATUS_LED_PIN
    digitalWrite(FIRE_STATUS_LED_PIN, HIGH);
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

    #ifdef HIT_STATUS_LED_PIN
    digitalWrite(HIT_STATUS_LED_PIN, HIGH);
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

  #ifdef FIRE_STATUS_LED_PIN
  if (millis() - lastFireTime > FIRE_STATUS_LED_DURATION_MILLIS) {
    digitalWrite(FIRE_STATUS_LED_PIN, LOW);
  }
  #endif

  #ifdef HIT_STATUS_LED_PIN
  if (millis() - lastHitTime > HIT_STATUS_LED_DURATION_MILLIS) {
    digitalWrite(HIT_STATUS_LED_PIN, LOW);
  }
  #endif

  #ifdef SPEAKER_PIN
  Sound::process(SPEAKER_PIN);
  #endif
}