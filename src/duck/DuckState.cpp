
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

    float g_zoom = 1.0f;

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
        , m_sensorListener()
        , m_ovenSensor()
        , m_spawnSensor()
        , m_killSensor()
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

        m_ovenSensor.SetDuckState(this);
        m_spawnSensor.SetDuckState(this);
        m_killSensor.SetDuckState(this);

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
        const float wallSize = 1.25f;
        // Convoyer belt
        for (int i = 0; i < 60; i++)
        {
            CreateWheel(vec2f(-30.0f + 0.25f + float(i) * 0.5f, PLAY_AREA_BOTTOM));
        }
        // Floor
        CreateStaticBox(vec2f(-16.0f, PLAY_AREA_BOTTOM - 4.0f), 0.0f, 16.0f, 4.0f);
        // Left wall at the start of convoyer belt
        CreateStaticBox(vec2f(PLAY_AREA_LEFT - 18.0f, 0.0f), 0.0f, wallSize, PLAY_AREA_H / 2.0f);

        // Left wall
        CreateStaticBox(vec2f(PLAY_AREA_LEFT - 1.0f, 4.0f), 0.0f, wallSize, PLAY_AREA_H / 2.0f);
        // Right wall
        CreateStaticBox(vec2f(PLAY_AREA_RIGHT + wallSize / 4.0f, -PLAY_AREA_H / 2.0f + 1.0f), 0.0f, wallSize / 2.0f, PLAY_AREA_H / 2.0f);
        // Ceiling
        CreateStaticBox(vec2f(0.0f, PLAY_AREA_TOP + 2.0f), 0.0f, PLAY_AREA_W / 2.0f, wallSize);

        // Bird slide
        CreateSlide();

        // Water container
        CreateWaterContainer(vec2f(0.0f, -2.0f), 2.0f, 0.15f);
        // Drying platform
        CreateStaticBox(vec2f(-8.0f, 4.0f), 0.0f, 2.0f, 0.5f);

        CreateOven(vec2f(PLAY_AREA_RIGHT / 2.0f, PLAY_AREA_BOTTOM));
        CreateSpawnArea(vec2f(PLAY_AREA_LEFT * 2.0f, 3.0f));


        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(8.0f, PLAY_AREA_BOTTOM - 16.0f);
        b2Body *body = m_world->CreateBody(&def);
        m_killSensor.SetBody(body);

        b2PolygonShape shape;
        shape.SetAsBox(2 * PLAY_AREA_W, 8.0f);
        m_killSensor.SetShape(&shape);
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
        wheel->SetBody(body);
//        wheel->SetColor(Color(0.2f, 0.2f, 0.2f));
        TextureHandle texture = GetCache().GetTexture("wheel.tex");
        wheel->SetTexture(texture);

        b2CircleShape shape;
        shape.m_radius = 0.25f;
        b2Fixture *fix = body->CreateFixture(&shape, 10.0f);
        b2Filter filter;
        filter.categoryBits = WheelBits;
        filter.maskBits &= ~StaticBits;
        filter.maskBits &= ~WheelBits;
        fix->SetFilterData(filter);

        b2RevoluteJointDef revDef;
        revDef.Initialize(body, m_worldBody, body->GetWorldCenter());
        revDef.enableMotor = true;
        revDef.motorSpeed = 30.0f;
        revDef.maxMotorTorque = 1000.0f;
        m_world->CreateJoint(&revDef);

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
        object->SetBody(body);
        object->SetColor(Color(0.65f, 0.63f, 0.65f));

        b2PolygonShape shape;
        shape.SetAsBox(w, h);
        b2Fixture *fix = body->CreateFixture(&shape, 1.0f);
        b2Filter filter;
        filter.categoryBits = StaticBits;
        fix->SetFilterData(filter);

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
        bodyDef.userData = (void*)object;
        b2Body *body = m_world->CreateBody(&bodyDef);

        shape.m_radius = 1.0f;
        b2FixtureDef fixDef;
        fixDef.shape = &shape;
        fixDef.userData = object;
        fixDef.density = 10.0f;
        fixDef.filter.categoryBits = BirdBits;
        body->CreateFixture(&fixDef);

        object->SetBody(body);
        TextureHandle texture = GetCache().GetTexture("bird_body.tex");
        object->SetTexture(texture);
        object->SetColor(Color(0.08f, 0.08f, 0.08f));

        GameObject *head = CreateObject(object);
        bodyDef.position = ToB2(position + vec2f(0.5f, 0.5f));
        b2Body *headBody = m_world->CreateBody(&bodyDef);

        shape.m_radius = 0.75f;
        headBody->CreateFixture(&shape, 5.0f);

        head->SetBody(headBody);
        texture = GetCache().GetTexture("bird_head.tex");
        head->SetTexture(texture);
        head->SetColor(Color(0.08f, 0.08f, 0.08f));

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
        neck0->SetColor(Color(0.08f, 0.08f, 0.08f));

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
        neck1->SetColor(Color(0.08f, 0.08f, 0.08f));

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
        neck2->SetColor(Color(0.08f, 0.08f, 0.08f));

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

    void DuckState::CreateOven(const vec2f &position)
    {
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&def);
        m_ovenSensor.SetBody(body);

        b2PolygonShape shape;
        shape.SetAsBox(PLAY_AREA_W / 4.0f, 1.0f);
        m_ovenSensor.SetShape(&shape);
    }

    void DuckState::CreateSpawnArea(const vec2f &position)
    {
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position = ToB2(position);
        b2Body *body = m_world->CreateBody(&def);
        m_spawnSensor.SetBody(body);

        b2PolygonShape shape;
        shape.SetAsBox(PLAY_AREA_W / 8.0f, PLAY_AREA_H / 2.0f);
        m_spawnSensor.SetShape(&shape);
    }

    void DuckState::CreateSlide()
    {
        GameObject *slide = CreateStaticBox(vec2f(PLAY_AREA_RIGHT + 2.0f, 0.0f), -30.0f * DEG2RAD_f, 8.0f, 0.5f);
        slide->GetBody()->GetFixtureList()->SetFriction(0.0f);

        b2BodyDef def;
        def.type = b2_staticBody;
        def.position.Set(PLAY_AREA_RIGHT + 8.0d, 2.0f);
        b2Body *body = m_world->CreateBody(&def);
        m_slideSensor.SetBody(body);

        b2PolygonShape shape;
        shape.SetAsBox(4.0f, 8.0f);
        m_slideSensor.SetShape(&shape);
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

    void DuckState::BirdGotBurned(GameObject *birdPart)
    {
        rob::log::Info("Bird got burned");
    }

    void DuckState::RecalcProj()
    {
        m_view.m_projection = Projection_Orthogonal_lh(PLAY_AREA_LEFT * g_zoom,
                                                       PLAY_AREA_RIGHT * g_zoom,
                                                       PLAY_AREA_BOTTOM * g_zoom,
                                                       PLAY_AREA_TOP * g_zoom, -1.0f, 1.0f);
    }

    void DuckState::OnResize(int w, int h)
    {
        const float x_scl = w / PLAY_AREA_W;
        const float y_scl = h / PLAY_AREA_H;
        const float scale = (x_scl < y_scl) ? x_scl : y_scl;

        const int vpW = scale * PLAY_AREA_W;
        const int vpH = scale * PLAY_AREA_H;
        m_view.SetViewport((w - vpW) / 2, (h - vpH) / 2, vpW, vpH);
        RecalcProj();
//        m_view.m_projection = Projection_Orthogonal_lh(PLAY_AREA_LEFT,
//                                                       PLAY_AREA_RIGHT,
//                                                       PLAY_AREA_BOTTOM,
//                                                       PLAY_AREA_TOP, -1.0f, 1.0f);
    }

    void DuckState::RealtimeUpdate(const Time_t deltaMicroseconds)
    { }

    void DuckState::NewBird()
    {
        if (m_spawnSensor.CanSpawn())
            CreateBird(vec2f(PLAY_AREA_LEFT * 2.0f, PLAY_AREA_BOTTOM + 4.0f));
    }

    void DuckState::Update(const GameTime &gameTime)
    {
        m_world->Step(gameTime.GetDeltaSeconds(), 8, 8);

        size_t deadCount = 0;
        GameObject *dead[MAX_OBJECTS];

        for (size_t i = 0; i < m_objectCount; i++)
        {
            if (m_objects[i]->IsAlive())
            {
                if (m_objects[i]->IsSaved())
                {
                    log::Info("Bird saved");
                    dead[deadCount++] = m_objects[i];
                }
            }
            else
            {
                dead[deadCount++] = m_objects[i];
            }
        }
        for (size_t i = 0; i < deadCount; i++)
        {
            DestroyObject(dead[i]);
        }
    }

    void DuckState::Render()
    {
        Renderer &renderer = GetRenderer();
        renderer.SetView(m_view);
        renderer.SetModel(mat4f::Identity);

        renderer.SetColor(Color(0.18f, 0.14f, 0.14f));
        renderer.BindColorShader();
        renderer.DrawFilledRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);

        float x0 = 0.0f;
        float y0 = PLAY_AREA_BOTTOM;
        float x1 = PLAY_AREA_RIGHT - 1.0f;
        float y1 = y0 + 4.0f;
        Color color0(1.0f, 1.0f, 0.0f, 0.7f);
        Color color1(1.0f, 0.0f, 0.0f, 0.0f);
        //renderer.SetModel(mat4f::Identity);
        //renderer.BindColorShader();
        renderer.DrawColorQuad(vec2f(x0, y0), color0, vec2f(x0 - 1.0f, y1), color1,
                               vec2f(x1, y1), color1, vec2f(x1 + 1.0f, y0), color0);

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
        if (key == Keyboard::Key::B)
            NewBird();
        if (key == Keyboard::Key::Space)
            ChangeState(STATE_Game);
        if (key == Keyboard::Key::Kp_Plus)
        {
            g_zoom = Clamp(g_zoom / 1.5f, 0.4444f, 4.0f);
            RecalcProj();
        }
        else if (key == Keyboard::Key::Kp_Minus)
        {
            g_zoom = Clamp(g_zoom * 1.5f, 0.4444f, 4.5f);
            RecalcProj();
        }
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
