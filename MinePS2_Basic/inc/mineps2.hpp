#pragma once

#include <tyra>
#include <vector>
#include "block.hpp"
#include "first_person_camera.hpp"

namespace MinePS2 {

class MinePS2Game : public Tyra::Game {
 public:
  explicit MinePS2Game(Tyra::Engine* engine);
  ~MinePS2Game();

  void init() override;
  void loop() override;

 private:
  static const int WORLD_X = 16;
  static const int WORLD_Y = 10;
  static const int WORLD_Z = 16;
  static constexpr float BLOCK_SIZE = 2.0F;
  static constexpr float BLOCK_HALF = 1.0F;
  static constexpr float EYE_HEIGHT = 1.6F;

  enum GameState { STATE_MENU, STATE_CONTROLS, STATE_ABOUT, STATE_PLAY, STATE_PAUSE };

  struct Cell {
    BlockType type;
  };

  Tyra::Engine* engine;
  Tyra::MinecraftPipeline voxelPipeline;
  Tyra::Texture* textureAtlas;
  FirstPersonCamera camera;

  Cell world[WORLD_X][WORLD_Y][WORLD_Z];
  std::vector<RenderBlock> visibleBlocks;
  std::vector<Tyra::McpipBlock*> renderList;

  Tyra::Sprite menuSprites[3];
  Tyra::Sprite controlsSprite;
  Tyra::Sprite aboutSprite;
  Tyra::Sprite pauseSprite;
  Tyra::Sprite hudSprites[6];

  GameState state;
  int selectedSlot;
  int menuSelection;
  float verticalVelocity;
  bool grounded;

  void loadAssets();
  void configureFullscreenSprite(Tyra::Sprite& sprite);
  void generateWorld();
  void rebuildVisibleBlocks();
  void addTree(int x, int groundY, int z);
  int terrainHeightAt(int x, int z) const;
  int highestSolidY(int x, int z) const;
  bool inBounds(int x, int y, int z) const;
  bool isSolid(int x, int y, int z) const;
  bool hasExposedFace(int x, int y, int z) const;

  void updateMenu();
  void updatePause();
  void updateInfoScreen();
  void updateGame();
  void updateMovement();
  void updateActions();
  void renderMenu();
  void renderPause();
  void renderControls();
  void renderAbout();
  void renderGame();

  void resetPlayer();
  float groundEyeY(float worldX, float worldZ) const;
  bool raycast(int& hitX, int& hitY, int& hitZ,
               int& placeX, int& placeY, int& placeZ) const;
  BlockType selectedBlockType() const;
};

}  // namespace MinePS2
