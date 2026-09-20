#include "engine.hpp"
#include "mineps2.hpp"

int main() {
  Tyra::Engine engine;
  MinePS2::MinePS2Game game(&engine);
  engine.run(&game);
  return 0;
}
