#pragma once

#include <tyra>

namespace MinePS2 {

class FirstPersonCamera {
 public:
  explicit FirstPersonCamera(Tyra::Pad* pad);

  void reset(const Tyra::Vec4& spawn);
  void updateLook();
  Tyra::CameraInfo3D getCameraInfo();
  Tyra::Vec4 getForward() const;
  Tyra::Vec4 getFlatForward() const;
  Tyra::Vec4 getRight() const;

  Tyra::Vec4 position;
  Tyra::Vec4 lookAt;
  float yaw;
  float pitch;

 private:
  Tyra::Pad* pad;
  void rebuildLookAt();
};

}  // namespace MinePS2
