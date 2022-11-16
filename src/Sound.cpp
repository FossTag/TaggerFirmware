#include <Arduino.h>
#include <Sound.h>

Sound* Sound::currentSound = nullptr;

void Sound::play(Sound *sound) {
  currentSound = sound;
  sound->start();
}

void Sound::process(int outputPin) {
  if (currentSound != nullptr) {
    if (!currentSound->processSound(outputPin)) {
      currentSound = nullptr;
    }
  }
}

RandomDescendingNoteSound::RandomDescendingNoteSound(
  int minNoteDuration,
  int maxNoteDuration,
  int noteCount,
  int maxFrequency,
  int minFrequency,
  int frequencyVariation
) {
  this->minNoteDuration = minNoteDuration;
  this->maxNoteDuration = maxNoteDuration;
  this->noteCount = noteCount;
  this->maxFrequency = maxFrequency;
  this->minFrequency = minFrequency;
  this->frequencyVariation = frequencyVariation;

  this->nextNoteTime = 0;
  this->currentNoteIndex = 0;
  this->frequencyStepPerNote = (maxFrequency - minFrequency) / noteCount;
}

RandomDescendingNoteSound::~RandomDescendingNoteSound() {}

void RandomDescendingNoteSound::start() {
  currentNoteIndex = 0;
  nextNoteTime = millis() - 1000; // Set this to the past so that the next call to processSound() will move to the first note.
}

bool RandomDescendingNoteSound::processSound(int speakerPin) {
  long time = millis();

  if (time > nextNoteTime) {
    currentNoteIndex ++;

    if (currentNoteIndex > noteCount) {
      noTone(speakerPin);
      return false;
    } else {
      int noteFrequency = rand() % frequencyVariation + (noteCount - currentNoteIndex) * frequencyStepPerNote;
      int noteDuration = rand() % (maxNoteDuration - minNoteDuration) + minNoteDuration;
      nextNoteTime = time + noteDuration;
      tone(speakerPin, noteFrequency, noteDuration);
    }
  }

  return true;
}
