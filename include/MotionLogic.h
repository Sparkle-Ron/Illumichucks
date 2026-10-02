#pragma once
#include <stdint.h>
// Pure logic: separate from IMU I/O for later gesture tuning.
class MotionLogic {
public:
  struct Event { bool moving; bool impact; float totalG; float motionG; };
  MotionLogic(float motionThreshold, float impactThreshold, uint32_t cooldown);
  void reset();
  Event update(float x, float y, float z, uint32_t now);
  static bool idleExpired(uint32_t now, uint32_t lastMovement, uint32_t timeout) {
    return uint32_t(now - lastMovement) >= timeout;
  }
private:
  float motionThreshold_, impactThreshold_;
  uint32_t cooldown_, lastImpactMs_ = 0;
  bool initialized_ = false, haveImpact_ = false;
  float baselineX_ = 0, baselineY_ = 0, baselineZ_ = 0;
};
