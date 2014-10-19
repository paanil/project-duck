
#ifndef H_DUCK_DUCK_STATE_H
#define H_DUCK_DUCK_STATE_H

#include "rob/application/GameState.h"
#include "rob/application/GameTime.h"
#include "rob/renderer/Renderer.h"
#include "rob/memory/Pool.h"

#include "GameObject.h"

namespace duck
{

    using rob::Time_t;
    using rob::GameTime;

    class DebugDraw;

    class DuckState : public rob::GameState
    {
    public:
        DuckState();
        ~DuckState();

        bool Initialize() override;
        void CreateWorld();

        void OnResize(int w, int h) override;

        GameObject* CreateStaticBox(const vec2f &position, float angle, float w, float h);
        GameObject* CreateBird(const vec2f &position);

        void DestroyObject(GameObject *object);
        void DestroyAllObjects();

        void RealtimeUpdate(const Time_t deltaMicroseconds) override;
        void Update(const GameTime &gameTime) override;
        void Render() override;

        void OnKeyPress(rob::Keyboard::Key key, rob::Keyboard::Scancode scancode, rob::uint32_t mods) override;

    private:
        rob::View m_view;
        b2World *m_world;
        DebugDraw *m_debugDraw;

        rob::Pool<GameObject> m_objectPool;
        GameObject **m_objects;
        size_t m_objectCount;

        GameObject *m_obj;
    };

} // duck

#endif // H_DUCK_DUCK_STATE_H
