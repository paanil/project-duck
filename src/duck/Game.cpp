
#include "Game.h"
#include "DuckState.h"

#include "rob/application/Window.h"
#include "rob/application/GameState.h"
#include "rob/renderer/TextLayout.h"
#include "rob/input/Keyboard.h"
#include "rob/time/Time.h"
#include "rob/Log.h"

namespace duck
{

    using namespace rob;

    class MenuState : public rob::GameState
    {
    public:
        MenuState(/*GameData &gameData*/)
            /*: m_gameData(gameData)*/
        { }

        bool Initialize() override
        {
            //m_gameData.m_score = -1;
            return true;
        }

        ~MenuState()
        { }

        void Render() override
        {
            Delay(20);

            Renderer &renderer = GetRenderer();
            renderer.SetView(GetDefaultView());
            renderer.SetColor(Color(1.0f, 1.0f, 1.0f));
            renderer.BindFontShader();

            const Viewport vp = renderer.GetView().m_viewport;
            TextLayout layout(renderer, vp.w / 2.0f, vp.h / 3.0f);

            renderer.SetFontScale(4.0f);
            layout.AddTextAlignC("Save the birds", 0.0f);
            layout.AddLine();

            renderer.SetFontScale(1.0f);
            layout.AddLine();
            layout.AddLine();

            renderer.SetFontScale(1.0f);
            layout.AddTextAlignR("[space]", -20.0f);
            layout.AddTextAlignL("- New game", 10.0f);
            layout.AddLine();
//            layout.AddLine();
//            layout.AddTextAlignR("[return]", -20.0f);
//            layout.AddTextAlignL("- High scores", 10.0f);
//            layout.AddLine();
            layout.AddTextAlignR("[esc]", -20.0f);
            layout.AddTextAlignL("- Quit", 10.0f);
            layout.AddLine();
        }

        void OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods) override
        {
            if (key == Keyboard::Key::Escape)
                QuitState();
            if (key == Keyboard::Key::Space)
                ChangeState(STATE_Info);
//            if (key == Keyboard::Key::Return)
//                ChangeState(3);
        }
    private:
//        GameData &m_gameData;
    };


    class InfoState : public rob::GameState
    {
    public:
        InfoState()
        { }

        bool Initialize() override
        {
            return true;
        }

        ~InfoState()
        { }

        void Render() override
        {
            Delay(20);

            Renderer &renderer = GetRenderer();
            renderer.SetView(GetDefaultView());
            renderer.SetColor(Color(1.0f, 1.0f, 1.0f));
            renderer.BindFontShader();

            const Viewport vp = renderer.GetView().m_viewport;
            TextLayout layout(renderer, vp.w / 2.0f, vp.h / 5.0f * 2.0f);

            renderer.SetFontScale(1.0f);
            layout.AddTextAlignC("Thousands of birds die in every oil accident.", 0.0f);
            layout.AddLine();
        }

        void OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods) override
        {
            if (key == Keyboard::Key::Escape)
                QuitState();
            if (key == Keyboard::Key::Space)
                ChangeState(STATE_Game);
        }
    private:
    };


    bool Game::Initialize()
    {
        m_window->SetTitle(
        #if defined(ROB_DEBUG)
            "Duck - Debug"
        #else
            "Duck"
        #endif // ROB_DEBUG
        );
        HandleStateChange(STATE_MainMenu);
//        HandleStateChange(STATE_Game);
        return true;
    }

    void Game::HandleStateChange(int state)
    {
        switch (state)
        {
        case STATE_NoChange:    break;

        case STATE_MainMenu:    ChangeState<MenuState>(/*m_gameData*/); break;
        case STATE_Game:        ChangeState<DuckState>(m_gameData); break;
        case STATE_Info:        ChangeState<InfoState>(); break;
        default:
            rob::log::Error("Invalid state change (", state, ")");
            break;
        }
    }

} // duck
