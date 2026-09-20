#include "mineps2.hpp"
#include <math.h>

namespace MinePS2 {

static int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

MinePS2Game::MinePS2Game(Tyra::Engine* t_engine)
    : engine(t_engine),
      textureAtlas(nullptr),
      camera(&t_engine->pad),
      state(STATE_MENU),
      selectedSlot(0),
      menuSelection(0),
      verticalVelocity(0.0F),
      grounded(true) {}

MinePS2Game::~MinePS2Game() {
  auto& repo = engine->renderer.getTextureRepository();
  if (textureAtlas != nullptr) repo.free(textureAtlas);
  for (int i = 0; i < 3; ++i) repo.freeBySprite(menuSprites[i]);
  repo.freeBySprite(controlsSprite);
  repo.freeBySprite(aboutSprite);
  repo.freeBySprite(pauseSprite);
  for (int i = 0; i < 6; ++i) repo.freeBySprite(hudSprites[i]);
}

void MinePS2Game::init() {
  engine->renderer.setClearScreenColor(Tyra::Color(108.0F, 178.0F, 235.0F));
  voxelPipeline.setRenderer(&engine->renderer.core);

  loadAssets();
  generateWorld();
  rebuildVisibleBlocks();
  resetPlayer();
}

void MinePS2Game::loop() {
  switch (state) {
    case STATE_MENU:
      updateMenu();
      renderMenu();
      break;
    case STATE_CONTROLS:
      updateInfoScreen();
      renderControls();
      break;
    case STATE_ABOUT:
      updateInfoScreen();
      renderAbout();
      break;
    case STATE_PAUSE:
      updatePause();
      renderPause();
      break;
    case STATE_PLAY:
    default:
      updateGame();
      renderGame();
      break;
  }
}

void MinePS2Game::configureFullscreenSprite(Tyra::Sprite& sprite) {
  const auto& settings = engine->renderer.core.getSettings();
  sprite.mode = Tyra::SpriteMode::MODE_STRETCH;
  sprite.position = Tyra::Vec2(0.0F, 0.0F);
  sprite.size = Tyra::Vec2(static_cast<float>(settings.getWidth()),
                           static_cast<float>(settings.getHeight()));
}

void MinePS2Game::loadAssets() {
  auto& repo = engine->renderer.getTextureRepository();

  textureAtlas = repo.add(Tyra::FileUtils::fromCwd("atlas.png"));

  for (int i = 0; i < 3; ++i) {
    configureFullscreenSprite(menuSprites[i]);
    char path[32];
    path[0] = 'm'; path[1] = 'e'; path[2] = 'n'; path[3] = 'u';
    path[4] = '_'; path[5] = static_cast<char>('0' + i); path[6] = '.';
    path[7] = 'p'; path[8] = 'n'; path[9] = 'g'; path[10] = '\0';
    auto* tex = repo.add(Tyra::FileUtils::fromCwd(path));
    tex->addLink(menuSprites[i].id);
  }

  configureFullscreenSprite(controlsSprite);
  auto* controlsTexture = repo.add(Tyra::FileUtils::fromCwd("controls.png"));
  controlsTexture->addLink(controlsSprite.id);

  configureFullscreenSprite(aboutSprite);
  auto* aboutTexture = repo.add(Tyra::FileUtils::fromCwd("about.png"));
  aboutTexture->addLink(aboutSprite.id);

  configureFullscreenSprite(pauseSprite);
  auto* pauseTexture = repo.add(Tyra::FileUtils::fromCwd("pause.png"));
  pauseTexture->addLink(pauseSprite.id);

  for (int i = 0; i < 6; ++i) {
    configureFullscreenSprite(hudSprites[i]);
    char path[32];
    path[0] = 'h'; path[1] = 'u'; path[2] = 'd'; path[3] = '_';
    path[4] = static_cast<char>('0' + i); path[5] = '.';
    path[6] = 'p'; path[7] = 'n'; path[8] = 'g'; path[9] = '\0';
    auto* tex = repo.add(Tyra::FileUtils::fromCwd(path));
    tex->addLink(hudSprites[i].id);
  }
}

int MinePS2Game::terrainHeightAt(int x, int z) const {
  unsigned int n = static_cast<unsigned int>(x * 92837111u) ^
                   static_cast<unsigned int>(z * 689287499u) ^ 0x5A17u;
  n ^= (n >> 13);
  n *= 1274126177u;
  int bump = static_cast<int>((n >> 29) & 3u);
  return 2 + (bump >= 2 ? 1 : 0);
}

void MinePS2Game::generateWorld() {
  for (int x = 0; x < WORLD_X; ++x)
    for (int y = 0; y < WORLD_Y; ++y)
      for (int z = 0; z < WORLD_Z; ++z)
        world[x][y][z].type = BLOCK_AIR;

  for (int x = 0; x < WORLD_X; ++x) {
    for (int z = 0; z < WORLD_Z; ++z) {
      const int top = terrainHeightAt(x, z);
      for (int y = 0; y <= top; ++y) {
        if (y == top) world[x][y][z].type = BLOCK_GRASS;
        else if (y >= top - 1) world[x][y][z].type = BLOCK_DIRT;
        else world[x][y][z].type = BLOCK_STONE;
      }
    }
  }

  for (int x = 1; x <= 4; ++x) {
    for (int z = 1; z <= 3; ++z) {
      int top = highestSolidY(x, z);
      if (top >= 0) world[x][top][z].type = BLOCK_SAND;
    }
  }

  addTree(4, highestSolidY(4, 8), 8);
  addTree(11, highestSolidY(11, 11), 11);
  addTree(12, highestSolidY(12, 4), 4);
}

void MinePS2Game::addTree(int x, int groundY, int z) {
  if (groundY < 0) return;
  for (int y = groundY + 1; y <= groundY + 3; ++y) {
    if (inBounds(x, y, z)) world[x][y][z].type = BLOCK_WOOD;
  }
  const int crownY = groundY + 4;
  for (int dx = -1; dx <= 1; ++dx) {
    for (int dz = -1; dz <= 1; ++dz) {
      for (int dy = -1; dy <= 1; ++dy) {
        int xx = x + dx, yy = crownY + dy, zz = z + dz;
        if (!inBounds(xx, yy, zz)) continue;
        if (dx == 0 && dz == 0 && dy < 0) continue;
        world[xx][yy][zz].type = BLOCK_LEAVES;
      }
    }
  }
}

bool MinePS2Game::inBounds(int x, int y, int z) const {
  return x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y && z >= 0 && z < WORLD_Z;
}

bool MinePS2Game::isSolid(int x, int y, int z) const {
  return inBounds(x, y, z) && world[x][y][z].type != BLOCK_AIR;
}

int MinePS2Game::highestSolidY(int x, int z) const {
  if (x < 0 || x >= WORLD_X || z < 0 || z >= WORLD_Z) return -1;
  for (int y = WORLD_Y - 1; y >= 0; --y)
    if (world[x][y][z].type != BLOCK_AIR) return y;
  return -1;
}

bool MinePS2Game::hasExposedFace(int x, int y, int z) const {
  return !isSolid(x + 1, y, z) || !isSolid(x - 1, y, z) ||
         !isSolid(x, y + 1, z) || !isSolid(x, y - 1, z) ||
         !isSolid(x, y, z + 1) || !isSolid(x, y, z - 1);
}

void MinePS2Game::rebuildVisibleBlocks() {
  visibleBlocks.clear();
  renderList.clear();
  visibleBlocks.reserve(WORLD_X * WORLD_Y * WORLD_Z / 2);
  renderList.reserve(WORLD_X * WORLD_Y * WORLD_Z / 2);

  for (int x = 0; x < WORLD_X; ++x) {
    for (int y = 0; y < WORLD_Y; ++y) {
      for (int z = 0; z < WORLD_Z; ++z) {
        BlockType type = world[x][y][z].type;
        if (type == BLOCK_AIR || !hasExposedFace(x, y, z)) continue;
        visibleBlocks.push_back(RenderBlock());
        visibleBlocks.back().configure(x, y, z, type);
      }
    }
  }

  for (unsigned int i = 0; i < visibleBlocks.size(); ++i)
    renderList.push_back(&visibleBlocks[i].renderData);
}

void MinePS2Game::resetPlayer() {
  int sx = WORLD_X / 2;
  int sz = 2;
  float spawnY = groundEyeY(sx * BLOCK_SIZE, sz * BLOCK_SIZE);
  camera.reset(Tyra::Vec4(sx * BLOCK_SIZE, spawnY, sz * BLOCK_SIZE));
  verticalVelocity = 0.0F;
  grounded = true;
}

float MinePS2Game::groundEyeY(float worldX, float worldZ) const {
  int gx = clampInt(static_cast<int>(floorf((worldX + BLOCK_HALF) / BLOCK_SIZE)), 0, WORLD_X - 1);
  int gz = clampInt(static_cast<int>(floorf((worldZ + BLOCK_HALF) / BLOCK_SIZE)), 0, WORLD_Z - 1);
  int gy = highestSolidY(gx, gz);
  if (gy < 0) return EYE_HEIGHT;
  return gy * BLOCK_SIZE + BLOCK_HALF + EYE_HEIGHT;
}

void MinePS2Game::updateMenu() {
  const auto& clicked = engine->pad.getClicked();
  if (clicked.DpadUp) menuSelection = (menuSelection + 2) % 3;
  if (clicked.DpadDown) menuSelection = (menuSelection + 1) % 3;

  if (clicked.Cross || clicked.Start) {
    if (menuSelection == 0) {
      state = STATE_PLAY;
      resetPlayer();
    } else if (menuSelection == 1) {
      state = STATE_CONTROLS;
    } else {
      state = STATE_ABOUT;
    }
  }
}

void MinePS2Game::updateInfoScreen() {
  if (engine->pad.getClicked().Circle || engine->pad.getClicked().Cross ||
      engine->pad.getClicked().Start) {
    state = STATE_MENU;
  }
}

void MinePS2Game::updatePause() {
  if (engine->pad.getClicked().Start || engine->pad.getClicked().Cross)
    state = STATE_PLAY;
  if (engine->pad.getClicked().Circle)
    state = STATE_MENU;
}

void MinePS2Game::updateGame() {
  if (engine->pad.getClicked().Start) {
    state = STATE_PAUSE;
    return;
  }
  camera.updateLook();
  updateMovement();
  updateActions();
}

void MinePS2Game::updateMovement() {
  const auto& joy = engine->pad.getLeftJoyPad();
  const auto& pressed = engine->pad.getPressed();
  const auto& clicked = engine->pad.getClicked();

  float lx = (static_cast<float>(joy.h) - 128.0F) / 128.0F;
  float ly = (static_cast<float>(joy.v) - 128.0F) / 128.0F;
  if (fabsf(lx) < 0.16F) lx = 0.0F;
  if (fabsf(ly) < 0.16F) ly = 0.0F;

  float speed = pressed.L3 ? 0.16F : 0.095F;
  Tyra::Vec4 forward = camera.getFlatForward();
  Tyra::Vec4 right = camera.getRight();

  Tyra::Vec4 candidate = camera.position;
  candidate += right * (lx * speed);
  candidate += forward * (-ly * speed);

  const float minX = 0.0F;
  const float maxX = (WORLD_X - 1) * BLOCK_SIZE;
  const float minZ = 0.0F;
  const float maxZ = (WORLD_Z - 1) * BLOCK_SIZE;
  if (candidate.x < minX) candidate.x = minX;
  if (candidate.x > maxX) candidate.x = maxX;
  if (candidate.z < minZ) candidate.z = minZ;
  if (candidate.z > maxZ) candidate.z = maxZ;

  const float floorY = groundEyeY(candidate.x, candidate.z);

  if (grounded && clicked.Cross) {
    verticalVelocity = 0.24F;
    grounded = false;
  }

  if (!grounded) {
    candidate.y += verticalVelocity;
    verticalVelocity -= 0.012F;
    if (candidate.y <= floorY) {
      candidate.y = floorY;
      verticalVelocity = 0.0F;
      grounded = true;
    }
  } else {
    candidate.y = floorY;
  }

  camera.position = candidate;
}

BlockType MinePS2Game::selectedBlockType() const {
  static const BlockType slots[6] = {
      BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE,
      BLOCK_SAND, BLOCK_WOOD, BLOCK_PLANKS};
  return slots[selectedSlot];
}

bool MinePS2Game::raycast(int& hitX, int& hitY, int& hitZ,
                          int& placeX, int& placeY, int& placeZ) const {
  Tyra::Vec4 dir = camera.getForward();
  Tyra::Vec4 p = camera.position;
  int lastX = -1, lastY = -1, lastZ = -1;

  for (int i = 0; i < 48; ++i) {
    p += dir * 0.25F;
    int gx = static_cast<int>(floorf((p.x + BLOCK_HALF) / BLOCK_SIZE));
    int gy = static_cast<int>(floorf((p.y + BLOCK_HALF) / BLOCK_SIZE));
    int gz = static_cast<int>(floorf((p.z + BLOCK_HALF) / BLOCK_SIZE));

    if (!inBounds(gx, gy, gz)) continue;
    if (gx == lastX && gy == lastY && gz == lastZ) continue;

    if (world[gx][gy][gz].type != BLOCK_AIR) {
      hitX = gx; hitY = gy; hitZ = gz;
      placeX = lastX; placeY = lastY; placeZ = lastZ;
      return true;
    }

    lastX = gx; lastY = gy; lastZ = gz;
  }
  return false;
}

void MinePS2Game::updateActions() {
  const auto& clicked = engine->pad.getClicked();

  if (clicked.L1) selectedSlot = (selectedSlot + 5) % 6;
  if (clicked.R1) selectedSlot = (selectedSlot + 1) % 6;

  int hx, hy, hz, px, py, pz;
  if (clicked.R2 && raycast(hx, hy, hz, px, py, pz)) {
    if (hy > 0) {
      world[hx][hy][hz].type = BLOCK_AIR;
      rebuildVisibleBlocks();
    }
  }

  if (clicked.L2 && raycast(hx, hy, hz, px, py, pz)) {
    if (inBounds(px, py, pz) && world[px][py][pz].type == BLOCK_AIR) {
      int playerX = static_cast<int>(floorf((camera.position.x + BLOCK_HALF) / BLOCK_SIZE));
      int playerY = static_cast<int>(floorf((camera.position.y + BLOCK_HALF) / BLOCK_SIZE));
      int playerZ = static_cast<int>(floorf((camera.position.z + BLOCK_HALF) / BLOCK_SIZE));
      if (!(px == playerX && pz == playerZ && (py == playerY || py == playerY - 1))) {
        world[px][py][pz].type = selectedBlockType();
        rebuildVisibleBlocks();
      }
    }
  }
}

void MinePS2Game::renderMenu() {
  auto& renderer = engine->renderer;
  renderer.beginFrame();
  renderer.renderer2D.render(menuSprites[menuSelection]);
  renderer.endFrame();
}

void MinePS2Game::renderControls() {
  auto& renderer = engine->renderer;
  renderer.beginFrame();
  renderer.renderer2D.render(controlsSprite);
  renderer.endFrame();
}

void MinePS2Game::renderAbout() {
  auto& renderer = engine->renderer;
  renderer.beginFrame();
  renderer.renderer2D.render(aboutSprite);
  renderer.endFrame();
}

void MinePS2Game::renderPause() {
  auto& renderer = engine->renderer;
  renderer.beginFrame();
  renderer.renderer2D.render(pauseSprite);
  renderer.endFrame();
}

void MinePS2Game::renderGame() {
  auto& renderer = engine->renderer;
  Tyra::CameraInfo3D camInfo = camera.getCameraInfo();
  renderer.beginFrame(camInfo);
  {
    renderer.renderer3D.usePipeline(voxelPipeline);
    if (!renderList.empty()) voxelPipeline.render(renderList, textureAtlas, false, false);
    renderer.renderer2D.render(hudSprites[selectedSlot]);
  }
  renderer.endFrame();
}

}  // namespace MinePS2
