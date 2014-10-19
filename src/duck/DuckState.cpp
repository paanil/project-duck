
#include "DuckState.h"
#include "GameState.h"
#include "B2DebugDraw.h"

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
        : m_view()
        , m_world(nullptr)
        , m_debugDraw(nullptr)
        , m_drawBox2D(false)
        , m_objectPool()
        , m_objects(nullptr)
        , m_objectCount(0)
    { }

    DuckState::~DuckState()
    {
        DestroyAllObjects();
        GetAllocator().del_object(m_world);
        GetAllocator().del_object(m_debugDraw);
    }

    bool DuckState::Initialize()
    {
        m_objectPool.SetMemory(GetAllocator().AllocateArray<GameObject>(MAX_OBJECTS), GetArraySize<GameObject>(MAX_OBJECTS));
        m_objects = GetAllocator().AllocateArray<GameObject*>(MAX_OBJECTS);

        m_debugDraw = GetAllocator().new_object<DebugDraw>(&GetRenderer());
        int32 flags = 0;
        flags += b2Draw::e_shapeBit;
        flags += b2Draw::e_jointBit;
        flags += b2Draw::e_aabbBit;
        flags += b2Draw::e_centerOfMassBit;
        m_debugDraw->SetFlags(flags);

        m_world = GetAllocator().new_object<b2World>(b2Vec2(0.0, -9.81f));
        m_world->SetDebugDraw(m_debugDraw);

        CreateBird(vec2f::Zero);
        CreateWorld();
        return true;
    }

    void DuckState::CreateWorld()
    {
        CreateStaticBox(vec2f(-12.0f, PLAY_AREA_BOTTOM), 0.0f, 11.0f, 1.0f);
        CreateStaticBox(vec2f(12.0f, 0.0f), -30.0f * DEG2RAD_f, 6.0f, 0.5f);
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
        renderer.SetModel(mat4f::Identity);

        renderer.SetColor(Color(0.25,0.2,0.2));
        renderer.BindColorShader();
        renderer.DrawFilledRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);

        for (size_t i = 0; i < m_objectCount; i++)
        {
            m_objects[i]->Render(&renderer);
        }

        if (m_drawBox2D)
        {
            renderer.SetModel(mat4f::Identity);
            renderer.BindColorShader();
            m_world->DrawDebugData();
        }
    }

    void DuckState::OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods)
    {
        if (key == Keyboard::Key::Tab)
            m_drawBox2D = !m_drawBox2D;
        if (key == Keyboard::Key::Space)
            ChangeState(STATE_Game);
    }

} // duck
