
#ifndef H_DUCK_GAME_H
#define H_DUCK_GAME_H

#include "rob/application/Game.h"
#include "GameState.h"

namespace duck
{

    class Game : public rob::Game
    {
    public:
        bool Initialize() override;
    protected:
        void HandleStateChange(int state) override;
    };

} // duck

#endif // H_DUCK_GAME_H

