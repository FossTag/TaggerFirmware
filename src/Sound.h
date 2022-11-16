#ifndef ARDUINO_LASER_TAG_SOUND_H
#define ARDUINO_LASER_TAG_SOUND_H

/**
 * We generate noises as a quick succession of increasingly lower pitched notes.
 * Parameters for these are:
 *  - The length of each note.
 *  - The total number of notes.
 *  - The start frequency and the end frequency.
 *  - The amount of variation in note duration.
 *  - The amount of variation in note frequency.
 * 
 * For example, a "hit by another player" sound that is a poor interpretation of a chiptune explosion may be:
 *  - Medium duration and low pitched.
 * 
 * Whereas a "hit too many times, you are now dead" sound will want to be distinguished by:
 *  - Higher frequency, many more notes, and a lot longer.
 *
 * This was originally prototyped with the following procedural (and blocking) code:
 * 
 * for (int i = 15; i > 5; i--) {
 *   int noteDuration = rand() % 30 + 30;
 *
 *    tone(SPEAKER_PIN, rand() % 50 + i * 10, noteDuration);
 *    delay(noteDuration);
 *    noTone(SPEAKER_PIN);
 *  }
 * 
 * But this will prevent all other actions taking place (e.g. button presses, IR signal receiving, etc)
 * for the duration of the loop due to the constant calls to delay().
 *
 * Instead, we now record where we are up to in the specific sound, and then each iteration of loop() we
  * will ask the sound if it wants to proceed to its next note.
 *
 * If so, the sound will then generate the next note frequency + duration and call tone(), or else if it
 * has played all its notes, it will call noTone() and report it is done.
 *
 */
class Sound {
  public:
    virtual ~Sound() {};
    virtual void start() {};
    virtual bool processSound(int speakerPin) = 0;

    static void play(Sound *sound);

    /**
     * Call once per loop() to check whether we need to switch to the next note
     * or stop playign alltogether.
     */
    static void process(int outputPin);

  private:
    static Sound *currentSound;
};

class RandomDescendingNoteSound : public Sound {
  public:
    RandomDescendingNoteSound(
      int minNoteDuration,
      int maxNoteDuration,
      int noteCount,
      int maxFrequency,
      int minFrequency,
      int frequencyVariation
    );

    virtual ~RandomDescendingNoteSound();

    void start();
    bool processSound(int speakerPin) override;

  private:
    long nextNoteTime;
    int currentNoteIndex;
    int minNoteDuration;
    int maxNoteDuration;
    int noteCount;
    int maxFrequency;
    int minFrequency;
    int frequencyStepPerNote;
    int frequencyVariation;
};

#endif