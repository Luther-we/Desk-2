#include "LedAnimations.h"

void bootBlueBounce(Adafruit_NeoPixel& strip,
                    const LampState&    state,
                    uint16_t            frameDelayMs)
{
  strip.clear();
  strip.show();

  // Aller
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.clear();
    strip.setPixelColor(i, strip.Color(state.r, state.g, state.b));
    strip.show();
    delay(frameDelayMs);
  }

  // Retour
  for (int i = strip.numPixels() - 2; i > 0; i--) {
    strip.clear();
    strip.setPixelColor(i, strip.Color(state.r, state.g, state.b));
    strip.show();
    delay(frameDelayMs);
  }

  strip.clear();
  strip.show();
}
