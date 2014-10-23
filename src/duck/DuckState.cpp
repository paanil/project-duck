
#include "DuckState.h"
#include "GameState.h"
#include "B2DebugDraw.h"

#include "rob/renderer/Renderer.h"
#include "rob/resource/MasterCache.h"
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

    struct Rect
    {
        float left, right;
        float bottom, top;
        bool IsInside(const vec2f &p) const
        {
            return (p.x >= left && p.x < right) &&
                (p.y >= bottom && p.y < top);
        }
    } g_playArea = {
        PLAY_AREA_LEFT, PLAY_AREA_RIGHT,
        PLAY_AREA_BOTTOM, PLAY_AREA_TOP
    };

    static const size_t MAX_OBJECTS = 1000;

    DuckState::DuckState()
        : m_view()
        , m_world(nullptr)
        , m_debugDraw(nullptr)
        , m_drawBox2D(false)
        , m_worldBody(nullptr)
        , m_mouseJoint(nullptr)
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

        m_world->SetContactListener(&m_sensorListener);

        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(0.0f, PLAY_AREA_BOTTOM - 2.0f);
        m_worldBody = m_world->CreateBody(&def);

        CreateWorld();
        CreateBird(vec2f::Zero);
        CreateBird(vec2f(-2.0f, 0.0f));
        CreateBird(vec2f(-1.0f, 0.0f));
        CreateBird(vec2f(1.0f, 0.0f));
        CreateBird(vec2f(-2.5f, 2.0f));
        CreateBird(vec2f(-4.5f, 4.0f));
        CreateBird(vec2f(8.5f, 6.0f));
        return true;
    }

    void DuckState::CreateWorld()
    {
        // Convoyer belt
//        CreateStaticBox(vec2f(-12.0f, PLAY_AREA_BOTTOM), 0.0f, 11.0f, 1.0f);
        for (int i = 0; i < 48; i++)
        {
            CreateWheel(vec2f(-12.0f + -12.0f + float(i) * 0.5f, PLAY_AREA_BOTTOM));
        }

        // "Output slide"
        CreateStaticBox(vec2f(12.0f, 0.0f), -30.0f * DEG2RAD_f, 5.0f, 0.25f);

        // Water container
        CreateWaterContainer(vec2f(0.0f, -2.0f), 2.0f, 0.15f);
        // Drying platform
        CreateStaticBox(vec2f(-8.0f, 4.0f), 0.0f, 2.0f, 0.5f);

        b2BodyDef bodyDef;
        bodyDef.type = b2_staticBody;
        bodyDef.position = b2Vec2(0.0f, 0.0f);
        b2Body *body = m_world->CreateBody(&bodyDef);
        m_duckSensor.SetBody(body);

        b2CircleShape shape;
        shape.m_radius = 1.0f;
        m_duckSensor.SetShape(&shape);
    }

    GameObject* DuckState::CreateObject(GameObject *prevLink /*= nullptr*/)
    {
        ROB_ASSERT(m_objectCount < MAX_OBJECTS);

        GameObject *object = m_objectPool.Obtain();
        if (prevLink) prevLink->SetNext(object);
        m_objects[m_objectCount++] = object;
        return object;
    }

    GameObject* DuckState::CreateWheel(const vec2f &position)
    {
        GameObject *wheel = CreateObject();

        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&bodyDef);

        b2CircleShape shape;
        shape.m_radius = 0.25f;
        body->CreateFixture(&shape, 10.0f);

        b2RevoluteJointDef revDef;
        revDef.Initialize(body, m_worldBody, body->GetWorldCenter());
        revDef.enableMotor = true;
        revDef.motorSpeed = 30.0f;
        revDef.maxMotorTorque = 1000.0f;
        m_world->CreateJoint(&revDef);

        wheel->SetBody(body);
        return wheel;
    }

    GameObject* DuckState::CreateWaterContainer(const vec2f &position, float w, float h)
    {
        GameObject *object = CreateObject();

        b2BodyDef def;
        def.type = b2_staticBody;
        def.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&def);

        b2PolygonShape shapeBottom;
        shapeBottom.SetAsBox(w, h, b2Vec2(0.0f, -1.0f), 0.0f);
        body->CreateFixture(&shapeBottom, 1.0f);

        const float sideW = w * 0.65f * 0.5f;
        const float sideX1 = w * 1.4f;
        const float sideX2 = w * 1.1f;
        const float sideY1 = 0.70f;
        const float sideY2 = -0.4f;
        b2PolygonShape shapeLeft;
        shapeLeft.SetAsBox(sideW, h, b2Vec2(-sideX1, sideY1), -78.0f * DEG2RAD_f);
        body->CreateFixture(&shapeLeft, 1.0f);
        shapeLeft.SetAsBox(sideW, h, b2Vec2(-sideX2, sideY2), -50.0f * DEG2RAD_f);
        body->CreateFixture(&shapeLeft, 1.0f);

        b2PolygonShape shapeRight;
        shapeRight.SetAsBox(sideW, h, b2Vec2(sideX1, sideY1), 78.0f * DEG2RAD_f);
        body->CreateFixture(&shapeRight, 1.0f);
        shapeRight.SetAsBox(sideW, h, b2Vec2(sideX2, sideY2), 50.0f * DEG2RAD_f);
        body->CreateFixture(&shapeRight, 1.0f);

        object->SetBody(body);
        TextureHandle texture = GetCache().GetTexture("container.tex");
        object->SetTexture(texture);
        object->SetLayer(1);

        return object;
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
        b2BodyDef bodyDef;
        b2CircleShape shape;

        GameObject *object = CreateObject();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&bodyDef);

        shape.m_radius = 1.0f;
        b2FixtureDef fixDef;
        fixDef.shape = &shape;
        fixDef.userData = object;
        fixDef.density = 10.0f;
        fixDef.filter.categoryBits = DuckBits;
        body->CreateFixture(&fixDef);
//        body->CreateFixture(&shape, 10.0f);

        object->SetBody(body);
        TextureHandle texture = GetCache().GetTexture("bird_body.tex");
        object->SetTexture(texture);
        object->SetColor(Color(0.25f, 0.25f, 0.25f));

        GameObject *head = CreateObject(object);
        bodyDef.position = ToB2(position + vec2f(0.5f, 0.5f));
        b2Body *headBody = m_world->CreateBody(&bodyDef);

        shape.m_radius = 0.75f;
        headBody->CreateFixture(&shape, 5.0f);

        head->SetBody(headBody);
        texture = GetCache().GetTexture("bird_head.tex");
        head->SetTexture(texture);

        // Neck
        b2PolygonShape neckShape;
        neckShape.SetAsBox(0.4f, 0.25f);
        TextureHandle neckTex = GetCache().GetTexture("bird_neck.tex");
        const float neckJlen = 0.25f;

        b2RevoluteJointDef neckJoint;
        neckJoint.collideConnected = false;

        GameObject *neck0 = CreateObject(head);
        bodyDef.position = ToB2(position + vec2f(0.85f, 0.85f));
        b2Body *neck0body = m_world->CreateBody(&bodyDef);
        neck0body->CreateFixture(&neckShape, 5.0f);
        neck0->SetBody(neck0body);
        neck0->SetTexture(neckTex);

        neckJoint.bodyA = body;
        neckJoint.bodyB = neck0body;
        neckJoint.localAnchorA.Set(0.5f, 0.5f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        m_world->CreateJoint(&neckJoint);

        GameObject *neck1 = CreateObject(neck0);
        bodyDef.position = ToB2(position + vec2f(1.0f, 1.0f));
        b2Body *neck1body = m_world->CreateBody(&bodyDef);
        neck1body->CreateFixture(&neckShape, 5.0f);
        neck1->SetBody(neck1body);
        neck1->SetTexture(neckTex);

        neckJoint.bodyA = neck0body;
        neckJoint.bodyB = neck1body;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        m_world->CreateJoint(&neckJoint);

        GameObject *neck2 = CreateObject(neck1);
        bodyDef.position = ToB2(position + vec2f(1.15f, 1.15f));
        b2Body *neck2body = m_world->CreateBody(&bodyDef);
        neck2body->CreateFixture(&neckShape, 5.0f);
        neck2->SetBody(neck2body);
        neck2->SetTexture(neckTex);

        neck2->SetNext(object);

        neckJoint.bodyA = neck1body;
        neckJoint.bodyB = neck2body;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        m_world->CreateJoint(&neckJoint);

        neckJoint.bodyA = neck2body;
        neckJoint.bodyB = headBody;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-0.4f, -0.4f);
        m_world->CreateJoint(&neckJoint);

        // Ropes
        b2RopeJointDef neckDef;
        neckDef.bodyA = body;
        neckDef.bodyB = headBody;
        neckDef.localAnchorA.Set(0.5f, 0.3f);
        neckDef.localAnchorB.Set(-0.1f, -0.2f);
        neckDef.maxLength = 2.5f;
        neckDef.collideConnected = true;
        m_world->CreateJoint(&neckDef);

        neckDef.bodyA = body;
        neckDef.bodyB = headBody;
        neckDef.localAnchorA.Set(0.3f, 0.5f);
        neckDef.localAnchorB.Set(-0.3f, -0.3f);
        neckDef.maxLength = 2.5f;
        neckDef.collideConnected = true;
        m_world->CreateJoint(&neckDef);

        return object;
    }

    void DuckState::DestroyObject(GameObject *object)
    {
        GameObject *next = object->GetNext();
        DestroySingleObject(object);
        if (next) DestroyObjectList(next, object);
    }

    void DuckState::DestroyObjectList(GameObject *object, GameObject *last)
    {
        GameObject *next = object->GetNext();
        DestroySingleObject(object);
        if (next && next != last) DestroyObjectList(next, last);
    }

    void DuckState::DestroySingleObject(GameObject *object)
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
    { }

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

        int maxLayer = 0;
        for (int layer = 0; layer < maxLayer + 1; layer++)
        {
            for (size_t i = 0; i < m_objectCount; i++)
            {
                const int l = m_objects[i]->GetLayer();
                if (l == layer)
                    m_objects[i]->Render(&renderer);
                if (l > maxLayer)
                    maxLayer = l;
            }
        }

        if (m_drawBox2D)
        {
            renderer.SetModel(mat4f::Identity);
            renderer.BindColorShader();
            m_world->DrawDebugData();
        }

        renderer.SetView(GetDefaultView());
        renderer.BindFontShader();
        renderer.SetColor(Color::White);

        char text[40];
        StringPrintF(text, "mouse=%f, %f", m_mouseWorld.x, m_mouseWorld.y);
        renderer.DrawText(0.0f, 0.0f, text);
    }

    void DuckState::OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods)
    {
        if (key == Keyboard::Key::Tab)
            m_drawBox2D = !m_drawBox2D;
        if (key == Keyboard::Key::Space)
            ChangeState(STATE_Game);
    }


    class QueryCallback : public b2QueryCallback
    {
    public:
        QueryCallback(const b2Vec2& point)
        {
            m_point = point;
            m_fixture = NULL;
        }

        bool ReportFixture(b2Fixture* fixture)
        {
            b2Body* body = fixture->GetBody();
            if (body->GetType() == b2_dynamicBody)
            {
                bool inside = fixture->TestPoint(m_point);
                if (inside)
                {
                        m_fixture = fixture;

                        // We are done, terminate the query.
                        return false;
                }
            }

            // Continue the query.
            return true;
        }

        b2Vec2 m_point;
        b2Fixture* m_fixture;
    };

    vec2f ScreenToWorld(const View &view, int x, int y)
    {
        const Viewport &viewport = view.m_viewport;
        const vec4f vp = vec4f(viewport.w, -viewport.h, 1.0f, 1.0f);
        const vec4f vp_pos = vec4f(x - viewport.x, y - viewport.y, 0.0f, 1.0f);

        const vec4f ndc = (vp_pos * 2.0f - vec4f(viewport.w, viewport.h, 0.0f, 0.0f)) / vp;
        const vec4f wp = Unproject_Orthogonal_lh(view.m_projection, ndc);
        return vec2f(wp.x, wp.y);
    }

    void DuckState::OnMouseDown(MouseButton button, int x, int y)
    {
        m_mouseWorld = ScreenToWorld(m_view, x, y);

        if (m_mouseJoint != nullptr)
            return;
        if (!g_playArea.IsInside(m_mouseWorld))
            return;

        // Make a small box.
        b2AABB aabb;
        b2Vec2 d;
        b2Vec2 p = ToB2(m_mouseWorld);
        d.Set(0.001f, 0.001f);
        aabb.lowerBound = p - d;
        aabb.upperBound = p + d;

        // Query the world for overlapping shapes.
        QueryCallback callback(p);
        m_world->QueryAABB(&callback, aabb);

        if (callback.m_fixture)
        {
            b2Body* body = callback.m_fixture->GetBody();
            b2MouseJointDef md;
            md.bodyA = m_worldBody;
            md.bodyB = body;
            md.target = p;
            md.maxForce = 1000.0f * body->GetMass();
            md.dampingRatio = 0.98f;
            m_mouseJoint = (b2MouseJoint*)m_world->CreateJoint(&md);
            body->SetAwake(true);
        }
    }

    void DuckState::OnMouseUp(MouseButton button, int x, int y)
    {
        if (m_mouseJoint)
        {
//            b2Body *body = m_mouseJoint->GetBodyB();
//            body->ApplyForceToCenter(-1000.0f * body->GetLinearVelocity(), false);
            m_world->DestroyJoint(m_mouseJoint);
            m_mouseJoint = nullptr;
        }
    }

    void DuckState::OnMouseMove(int x, int y)
    {
        m_mouseWorld = ScreenToWorld(m_view, x, y);
        if (m_mouseJoint)
        {
            if (g_playArea.IsInside(m_mouseWorld))
            {
                const b2Vec2 target(m_mouseWorld.x, m_mouseWorld.y);
                m_mouseJoint->SetTarget(target);
            }
//            else
//            {
//                m_world->DestroyJoint(m_mouseJoint);
//                m_mouseJoint = nullptr;
//            }
        }
    }

} // duck
