
#include "DuckState.h"
#include "GameState.h"

#include "rob/renderer/Renderer.h"
#include "rob/math/Projection.h"

namespace duck
{

    using namespace rob;

    static const float PLAY_AREA_W      = 24.0f;
    static const float PLAY_AREA_H      = PLAY_AREA_W * 0.75f;
    static const float PLAY_AREA_LEFT   = -PLAY_AREA_W / 2.0f;
    static const float PLAY_AREA_RIGHT  = -PLAY_AREA_LEFT;
    static const float PLAY_AREA_BOTTOM = -PLAY_AREA_H / 2.0f;
    static const float PLAY_AREA_TOP    = -PLAY_AREA_BOTTOM;

    DuckState::DuckState()
        : m_world(nullptr)
        , m_obj(nullptr)
        , m_floor(nullptr)
    { }

    DuckState::~DuckState()
    {
        GetAllocator().del_object(m_obj);
        GetAllocator().del_object(m_floor);
        GetAllocator().del_object(m_world);
    }

    bool DuckState::Initialize()
    {
        m_world = GetAllocator().new_object<b2World>(b2Vec2(0.0, -9.81f));
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.Set(0.0f, 0.0f);
        b2Body *body = m_world->CreateBody(&bodyDef);

        b2CircleShape circleShape;
        circleShape.m_radius = 1.0f;
        b2Fixture *circle = body->CreateFixture(&circleShape, 1.0f);
        circle->SetRestitution(0.6f);

        m_obj = GetAllocator().new_object<GameObject>();
        m_obj->SetBody(body);

        b2BodyDef floorDef;
        floorDef.type = b2_staticBody;
        floorDef.position.Set(0.0f, PLAY_AREA_BOTTOM);
        b2Body *floor = m_world->CreateBody(&floorDef);

        b2PolygonShape floorShape;
        floorShape.SetAsBox(PLAY_AREA_W, 1.0f);
        floor->CreateFixture(&floorShape, 1.0f);

        m_floor = GetAllocator().new_object<GameObject>();
        m_floor->SetBody(floor);

        return true;
    }

    void DuckState::OnResize(int w, int h)
    {
        const float x_scl = w / PLAY_AREA_W;
        const float y_scl = h / PLAY_AREA_H;
        const float scale = (x_scl < y_scl) ? x_scl : y_scl;

        const int vpW = scale * PLAY_AREA_W;
        const int vpH = scale * PLAY_AREA_H;
        m_view.SetViewport((w - vpW) / 2, (h - vpH) / 2, vpW, vpH);
        m_view.m_projection = Projection_Orthogonal_lh(PLAY_AREA_LEFT,
                                                       PLAY_AREA_RIGHT,
                                                       PLAY_AREA_BOTTOM,
                                                       PLAY_AREA_TOP, -1.0f, 1.0f);
    }

    void DuckState::RealtimeUpdate(const Time_t deltaMicroseconds)
    {

    }

    void DuckState::Update(const GameTime &gameTime)
    {
        m_world->Step(gameTime.GetDeltaSeconds(), 8, 8);
    }

    void DuckState::Render()
    {
        Renderer &renderer = GetRenderer();
        renderer.SetView(m_view);
        m_obj->Render(&renderer);
        m_floor->Render(&renderer);
    }

    void DuckState::OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods)
    {
        if (key == Keyboard::Key::Space)
            ChangeState(STATE_Game);
    }

} // duck
