#include "MotionLogic.h"
#include <math.h>
MotionLogic::MotionLogic(float motionThreshold, float impactThreshold, uint32_t cooldown)
 : motionThreshold_(motionThreshold), impactThreshold_(impactThreshold), cooldown_(cooldown) {}
void MotionLogic::reset() { initialized_ = false; haveImpact_ = false; }
MotionLogic::Event MotionLogic::update(float x, float y, float z, uint32_t now) {
  const float total = sqrtf(x*x + y*y + z*z);
  if (!initialized_) {
    baselineX_ = x; baselineY_ = y; baselineZ_ = z; initialized_ = true;
    // First sample seeds the gravity baseline, rather than generating a gesture.
    return {false, false, total, 0};
  }
  const float dx = x - baselineX_, dy = y - baselineY_, dz = z - baselineZ_;
  const float motion = sqrtf(dx*dx + dy*dy + dz*dz);
  // Slowly follow gravity/orientation; unlike |acceleration|-1 this detects tilts.
  constexpr float alpha = 0.02f; // About 0.24s at the active 208Hz sensor rate.
  baselineX_ += alpha*dx; baselineY_ += alpha*dy; baselineZ_ += alpha*dz;
  const bool impact = total >= impactThreshold_ &&
                     (!haveImpact_ || uint32_t(now - lastImpactMs_) >= cooldown_);
  if (impact) { haveImpact_ = true; lastImpactMs_ = now; }
  return {motion >= motionThreshold_, impact, total, motion};
}
