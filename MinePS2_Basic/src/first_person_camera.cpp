#include "first_person_camera.hpp"
#include <math.h>

namespace MinePS2 {

FirstPersonCamera::FirstPersonCamera(Tyra::Pad* t_pad)
    : position(0.0F), lookAt(0.0F), yaw(0.0F), pitch(0.0F), pad(t_pad) {}

void FirstPersonCamera::reset(const Tyra::Vec4& spawn) {
  position = spawn;
  yaw = 0.0F;
  pitch = 0.0F;
  rebuildLookAt();
}

void FirstPersonCamera::updateLook() {
  const auto& joy = pad->getRightJoyPad();
  const float dead = 18.0F;
  const float center = 128.0F;
  float x = (static_cast<float>(joy.h) - center) / center;
  float y = (static_cast<float>(joy.v) - center) / center;

  if (fabsf(x) < dead / center) x = 0.0F;
  if (fabsf(y) < dead / center) y = 0.0F;

  yaw += x * 0.045F;
  pitch -= y * 0.035F;

  if (pitch > 1.25F) pitch = 1.25F;
  if (pitch < -1.25F) pitch = -1.25F;
  rebuildLookAt();
}

Tyra::Vec4 FirstPersonCamera::getForward() const {
  const float cp = Tyra::Math::cos(pitch);
  Tyra::Vec4 f(Tyra::Math::sin(yaw) * cp,
               Tyra::Math::sin(pitch),
               Tyra::Math::cos(yaw) * cp);
  f.normalize();
  return f;
}

Tyra::Vec4 FirstPersonCamera::getFlatForward() const {
  Tyra::Vec4 f(Tyra::Math::sin(yaw), 0.0F, Tyra::Math::cos(yaw));
  f.normalize();
  return f;
}

Tyra::Vec4 FirstPersonCamera::getRight() const {
  Tyra::Vec4 r(Tyra::Math::cos(yaw), 0.0F, -Tyra::Math::sin(yaw));
  r.normalize();
  return r;
}

void FirstPersonCamera::rebuildLookAt() {
  lookAt = position + getForward() * 10.0F;
}

Tyra::CameraInfo3D FirstPersonCamera::getCameraInfo() {
  rebuildLookAt();
  return Tyra::CameraInfo3D(&position, &lookAt);
}

}  // namespace MinePS2
