
#ifndef H_DUCK_DUCK_STATE_H
#define H_DUCK_DUCK_STATE_H

#include "rob/application/GameState.h"
#include "rob/application/GameTime.h"
#include "rob/renderer/Renderer.h"
#include "Physics.h"
#include "GameObject.h"

namespace duck
{

    using rob::Time_t;
    using rob::GameTime;

    class DuckState : public rob::GameState
    {
    public:
        DuckState();
        ~DuckState();

        bool Initialize() override;

        void OnResize(int w, int h) override;

        void RealtimeUpdate(const Time_t deltaMicroseconds) override;
        void Update(const GameTime &gameTime) override;
        void Render() override;

        void OnKeyPress(rob::Keyboard::Key key, rob::Keyboard::Scancode scancode, rob::uint32_t mods) override;

    private:
        rob::View m_view;
        b2World *m_world;

        GameObject *m_obj;
        GameObject *m_floor;
    };

} // duck

#endif // H_DUCK_DUCK_STATE_H

