
#include <SDL2/SDL.h>

#include "bacteroids/BacteroidsGame.h"

#ifdef ROB_DEBUG
#include "resource/Builder/MasterBuilder.h"
#endif

int main(int argc, char *argv[]) __attribute__((force_align_arg_pointer));

int main(int argc, char *argv[])
{
#ifdef ROB_DEBUG
    rob::MasterBuilder builder;
    builder.Build("data_source", "data");
#endif

    bact::Bacteroids game;
    game.Run();

    return 0;
}
