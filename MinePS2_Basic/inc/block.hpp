#pragma once

#include <tyra>

namespace MinePS2 {

enum BlockType : unsigned char {
  BLOCK_AIR = 0,
  BLOCK_GRASS,
  BLOCK_DIRT,
  BLOCK_STONE,
  BLOCK_SAND,
  BLOCK_WOOD,
  BLOCK_LEAVES,
  BLOCK_PLANKS
};

class RenderBlock {
 public:
  RenderBlock();
  RenderBlock(const RenderBlock& other);
  ~RenderBlock() = default;

  void configure(int gx, int gy, int gz, BlockType type);
  void updateModelMatrix();

  Tyra::M4x4 translation;
  Tyra::M4x4 rotation;
  Tyra::M4x4 scale;
  Tyra::M4x4 model;
  Tyra::Color color;
  Tyra::Vec4 atlasOffset;
  Tyra::McpipBlock renderData;

 private:
  void bindRenderData();
};

}  // namespace MinePS2
