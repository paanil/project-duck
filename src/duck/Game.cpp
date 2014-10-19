
#include "Game.h"
#include "DuckState.h"

#include "rob/application/Window.h"
#include "rob/Log.h"

namespace duck
{

    bool Game::Initialize()
    {
        m_window->SetTitle(
        #if defined(ROB_DEBUG)
            "Duck - Debug"
        #else
            "Duck"
        #endif // ROB_DEBUG
        );
        HandleStateChange(STATE_Game);
        return true;
    }

    void Game::HandleStateChange(int state)
    {
        switch (state)
        {
        case STATE_NoChange:    break;

        case STATE_MainMenu:    break; //ChangeState<MenuState>(m_gameData); break;
        case STATE_Game:        ChangeState<DuckState>(); break;
        default:
            rob::log::Error("Invalid state change (", state, ")");
            break;
        }
    }

} // duck
