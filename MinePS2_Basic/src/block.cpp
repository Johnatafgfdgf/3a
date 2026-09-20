#include "block.hpp"

namespace MinePS2 {

static void atlasFor(BlockType type, unsigned char& x, unsigned char& y) {
  y = 0;
  switch (type) {
    case BLOCK_GRASS:  x = 0; break;
    case BLOCK_DIRT:   x = 1; break;
    case BLOCK_STONE:  x = 2; break;
    case BLOCK_SAND:   x = 3; break;
    case BLOCK_WOOD:   x = 4; break;
    case BLOCK_LEAVES: x = 5; break;
    case BLOCK_PLANKS: x = 6; break;
    default:           x = 1; break;
  }
}

RenderBlock::RenderBlock() {
  translation = Tyra::M4x4::Identity;
  rotation = Tyra::M4x4::Identity;
  scale = Tyra::M4x4::Identity;
  model = Tyra::M4x4::Identity;
  color = Tyra::Color(128.0F, 128.0F, 128.0F, 128.0F);
  atlasOffset = Tyra::Vec4(0.0F);
  bindRenderData();
}

RenderBlock::RenderBlock(const RenderBlock& other) {
  translation = other.translation;
  rotation = other.rotation;
  scale = other.scale;
  model = other.model;
  color = other.color;
  atlasOffset = other.atlasOffset;
  bindRenderData();
}

void RenderBlock::bindRenderData() {
  renderData.color = &color;
  renderData.model = &model;
  renderData.textureOffset = &atlasOffset;
}

void RenderBlock::configure(int gx, int gy, int gz, BlockType type) {
  translation = Tyra::M4x4::Identity;
  rotation = Tyra::M4x4::Identity;
  scale = Tyra::M4x4::Identity;

  translation.translateX(static_cast<float>(gx) * 2.0F);
  translation.translateY(static_cast<float>(gy) * 2.0F);
  translation.translateZ(static_cast<float>(gz) * 2.0F);

  unsigned char atlasX = 0, atlasY = 0;
  atlasFor(type, atlasX, atlasY);
  const float tile = 1.0F / 16.0F;
  atlasOffset = Tyra::Vec4(atlasX * tile, atlasY * tile);

  color = Tyra::Color(128.0F, 128.0F, 128.0F, 128.0F);
  updateModelMatrix();
}

void RenderBlock::updateModelMatrix() {
  model = translation * rotation * scale;
}

}  // namespace MinePS2
