
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

    static const size_t MAX_OBJECTS = 1000;

    DuckState::DuckState()
        : m_world(nullptr)
        , m_objectPool()
        , m_objects(nullptr)
        , m_objectCount(0)
        , m_obj(nullptr)
        , m_floor(nullptr)
    { }

    DuckState::~DuckState()
    {
        DestroyAllObjects();
        GetAllocator().del_object(m_world);
    }

    bool DuckState::Initialize()
    {
        m_objectPool.SetMemory(GetAllocator().AllocateArray<GameObject>(MAX_OBJECTS), GetArraySize<GameObject>(MAX_OBJECTS));
        m_objects = GetAllocator().AllocateArray<GameObject*>(MAX_OBJECTS);

        m_world = GetAllocator().new_object<b2World>(b2Vec2(0.0, -9.81f));

        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.Set(0.0f, 0.0f);
        b2Body *body = m_world->CreateBody(&bodyDef);

        b2CircleShape circleShape;
        circleShape.m_radius = 1.0f;
        b2Fixture *circle = body->CreateFixture(&circleShape, 1.0f);
        circle->SetRestitution(0.6f);

        m_obj = CreateBird(vec2f::Zero);
        m_floor = CreateStaticBox(vec2f(0.0f, PLAY_AREA_BOTTOM), 0.0f, PLAY_AREA_W, 1.0f);
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

    GameObject* DuckState::CreateStaticBox(const vec2f &position, float angle, float w, float h)
    {
        GameObject *object = m_objectPool.Obtain();

        b2BodyDef def;
        def.type = b2_staticBody;
        def.position = ToB2(position);
        def.angle = angle;
        b2Body *body = m_world->CreateBody(&def);

        b2PolygonShape shape;
        shape.SetAsBox(w, h);
        body->CreateFixture(&shape, 1.0f);

        object->SetBody(body);
        m_objects[m_objectCount++] = object;
        return object;
    }

    GameObject* DuckState::CreateBird(const vec2f &position)
    {
        GameObject *object = m_objectPool.Obtain();

        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&def);

        b2CircleShape shape;
        shape.m_radius = 1.0f;
        body->CreateFixture(&shape, 1.0f);

        object->SetBody(body);
        m_objects[m_objectCount++] = object;
        return object;
    }

    void DuckState::DestroyObject(GameObject *object)
    {
        for (size_t i = 0; i < m_objectCount; i++)
        {
            if (m_objects[i] == object)
            {
                m_objects[i] = (i < MAX_OBJECTS - 1) ?
                    m_objects[m_objectCount - 1] : nullptr;
                m_objectCount--;

                m_world->DestroyBody(object->GetBody());
                m_objectPool.Return(object);
                return;
            }
        }
        ROB_ASSERT(0);
    }

    void DuckState::DestroyAllObjects()
    {
        for (size_t i = 0; i < m_objectCount; i++)
        {
            m_world->DestroyBody(m_objects[i]->GetBody());
            m_objectPool.Return(m_objects[i]);
            m_objects[i] = nullptr;
        }
        m_objectCount = 0;
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

        for (size_t i = 0; i < m_objectCount; i++)
        {
            m_objects[i]->Render(&renderer);
        }
    }

    void DuckState::OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods)
    {
        if (key == Keyboard::Key::Space)
            ChangeState(STATE_Game);
        if (key == Keyboard::Key::R)
        {
            if (m_floor)
            {
                DestroyObject(m_floor);
                m_floor = nullptr;
            }
        }
    }

} // duck
