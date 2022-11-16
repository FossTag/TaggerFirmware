# FossTag/Tagger

A proof of concept laser-tag system for supported micro controllers.

# Currently supported

## Firing and getting hit

Each device can have a single button for firing. When pressed, will send an IR signal (using [Android-IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote)).

Currently the signal just includes information about who fired (e.g. to exclude hits from your own team, or hits from your own IR signals reflected back), the amount of damage to administer, and potentially other information.

# Software configuration

Strongly recommended:
* `FIRE_BUTTON_PIN`
* `FIRE_IR_SEND_PIN`
* `HIT_IR_RECEIVE_PIN`

Feedback:
* `SPEAKER_PIN`
* `FIRE_STATUS_LED_PIN`
* `HIT_STATUS_LED_PIN`

Tweaks:
* `FIRE_COOLOFF_MILLIS`
* `FIRE_STATUS_LED_DURATION_MILLIS`
* `HIT_STATUS_LED_DURATION_MILLIS`

# Hardware requirements

## Fire button

A momentary switch with one side connected to ground and the other `FIRE_BUTTON_PIN`.
This uses an internal pullup resistor, so no external resistors are required.

Technically optional (but highlighly recommended or else your device cannot fire).
The reason it was left optional was so that the same code could be used on static objects, e.g. firing range targets.

## IR LED

Recommendations:
 * Choose an appropriate lens to mount in front of the LED to increase range and make it more accurate.
 * Select a high current LED for further range.
 * Prefer LEDs with a narrow viewing angle so energy is not wasted firing sideways.

Technically optional (but highlighly recommended or else your device cannot fire).
The reason it was left optional was so that the same code could be used on static objects, e.g. firing range targets.

## IR Receiver

Document circuit used to allow multiple sensors in series.

## Speaker

## Status LEDs

Both firing and getting hit will can show momentary feedback via LEDs. 

# Wishlist

## Ammunition and reloading

## Secondary fire button

## Health and respawning

Each device should keep track of how much health it has remaining.

 * Getting hit by other guns (with audio + LED feedback)
 * Health / respawning

## Device configuration

Each device will need to be able to be configured dynamically, including:
* What player is using the gun? (e.g. for high scores)
* What type of gun is it? (e.g. slow shooting but large damage vs rapid fire)
* What team is the player on? (e.g. to avoid being hit)

## Visual feedback about which team you are on

e.g. RGB LED which shows to all around which team you are on.
