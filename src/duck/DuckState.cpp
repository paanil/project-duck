
#include "DuckState.h"
#include "GameState.h"
#include "B2DebugDraw.h"

#include "Logic.h"

#include "rob/renderer/Renderer.h"
#include "rob/renderer/TextLayout.h"
#include "rob/graphics/Graphics.h"
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

    static const float MAX_LIVES = 5;

    static const size_t MAX_OBJECTS = 1000;

    static const float SCORE_TIME = 1.0f; // seconds

    DuckState::DuckState(GameData &gameData)
        : m_gameData(gameData)
        , m_view()
        , m_world(nullptr)
        , m_debugDraw(nullptr)
        , m_drawBox2D(false)
        , m_inUpdate(false)
        , m_worldBody(nullptr)
        , m_mouseJoint(nullptr)
        , m_objectPool()
        , m_objects(nullptr)
        , m_objectCount(0)
        , m_sensorListener()
        , m_ovenSensor()
        , m_spawnSensor()
        , m_killSensor()
        , m_waterSensor()
        , m_fadeEffect(Color(0.04f, 0.01f, 0.01f))
        , m_scoreTimer(0.0f)
    {
        m_gameData.m_birdsKilled = 0;
        m_gameData.m_birdsSaved = 0;
        m_gameData.m_score = 0;
    }

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
        flags += b2Draw::e_particleBit;
        m_debugDraw->SetFlags(flags);

        m_world = GetAllocator().new_object<b2World>(b2Vec2(0.0, -9.81f));
        m_world->SetDebugDraw(m_debugDraw);

        m_world->SetContactListener(&m_sensorListener);

        b2BodyDef wbodyDef;
        wbodyDef.type = b2_staticBody;
        wbodyDef.position.Set(0.0f, PLAY_AREA_BOTTOM - 2.0f);
        m_worldBody = m_world->CreateBody(&wbodyDef);

        b2ParticleSystemDef wasteDef;
        wasteDef.radius = 0.2f;
        wasteDef.density = 1.0f;
        wasteDef.maxCount = 200;
        m_waste = m_world->CreateParticleSystem(&wasteDef);

        m_ovenSensor.SetDuckState(this);
        m_spawnSensor.SetDuckState(this);
        m_slideSensor.SetDuckState(this);
        m_killSensor.SetDuckState(this);
        m_waterSensor.SetDuckState(this);

        m_sounds.Init(GetAudio(), GetCache());

        CreateWorld();
        CreateBird(vec2f::Zero);
        CreateBird(vec2f(-2.0f, 0.0f));
        CreateBird(vec2f(-1.0f, 0.0f));
        CreateBird(vec2f(1.0f, 0.0f));
//        CreateBird(vec2f(-2.5f, 2.0f));
//        CreateBird(vec2f(-4.5f, 4.0f));
//        CreateBird(vec2f(8.5f, 6.0f));
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
        // Left wall at the start of conveyor belt
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

        CreateOven();
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
        b2Fixture *fix = body->CreateFixture(&shape, 200.0f);
        b2Filter filter;
        filter.categoryBits = WheelBits;
        filter.maskBits &= ~StaticBits;
        filter.maskBits &= ~WheelBits;
        fix->SetFilterData(filter);
        fix->SetFriction(2.0f);

        b2RevoluteJointDef revDef;
        revDef.Initialize(body, m_worldBody, body->GetWorldCenter());
        revDef.enableMotor = true;
        revDef.motorSpeed = 15.0f;
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
        TextureHandle texture = GetCache().GetTexture("container2.tex");
        object->SetTexture(texture);
        object->SetLayer(2);
        object->SetColor(Color(1.0f, 1.0f, 1.0f, 0.8f));

        // Sensor
        b2BodyDef def2;
        def2.type = b2_staticBody;
        def2.position = ToB2(position);
        b2Body *body2 = m_world->CreateBody(&def2);
        m_waterSensor.SetBody(body2);

        b2PolygonShape shape;
        shape.SetAsBox(w, h * 2.0f);
        m_waterSensor.SetShape(&shape);

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
        TextureHandle flameTexture = GetCache().GetTexture("flame.tex");
        TextureHandle flameGlowTexture = GetCache().GetTexture("flame_glow.tex");
        TextureHandle bubbleTexture = GetCache().GetTexture("bubbles.tex");

        b2BodyDef bodyDef;
        b2CircleShape shape;

        // Body
        GameObject *bird = CreateObject();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = ToB2(position);
        bodyDef.userData = (void*)bird;
        b2Body *body = m_world->CreateBody(&bodyDef);

        shape.m_radius = 1.0f;
        b2FixtureDef fixDef;
        fixDef.shape = &shape;
        fixDef.userData = bird;
        fixDef.density = 10.0f;
        fixDef.filter.categoryBits = BirdBits;
        body->CreateFixture(&fixDef);

        bird->SetBody(body);
        TextureHandle texture = GetCache().GetTexture("bird_body.tex");
        bird->SetTexture(texture);
        bird->SetFlameTexture(flameTexture);
        bird->SetFlameGlowTexture(flameGlowTexture);
        bird->SetBubbleTexture(bubbleTexture);
        bird->SetOily();
        bird->SetLayer(1);

        // Head
        GameObject *head = CreateObject(bird);
        bodyDef.position = ToB2(position + vec2f(0.5f, 0.5f));
        b2Body *headBody = m_world->CreateBody(&bodyDef);

        shape.m_radius = 0.75f;
        fixDef.shape = &shape;
        fixDef.userData = head;
        fixDef.density = 5.0f;
        fixDef.filter.categoryBits = BirdBits;
        headBody->CreateFixture(&fixDef);

        head->SetBody(headBody);
        texture = GetCache().GetTexture("bird_head.tex");
        head->SetTexture(texture);
        head->SetFlameTexture(flameTexture);
        head->SetFlameGlowTexture(flameGlowTexture);
        head->SetBubbleTexture(bubbleTexture);
        head->SetOily();
        head->SetLayer(1);

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
        neck0->SetFlameTexture(flameTexture);
        neck0->SetFlameGlowTexture(flameGlowTexture);
        neck0->SetBubbleTexture(bubbleTexture);
        neck0->SetOily();

        neckJoint.bodyA = body;
        neckJoint.bodyB = neck0body;
        neckJoint.localAnchorA.Set(0.5f, 0.5f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        neckJoint.enableMotor = true;
        neckJoint.motorSpeed = 0.0f;
        neckJoint.maxMotorTorque = 200.0f;
        b2RevoluteJoint* neckj = (b2RevoluteJoint*)m_world->CreateJoint(&neckJoint);

        GameObject *neck1 = CreateObject(neck0);
        bodyDef.position = ToB2(position + vec2f(1.0f, 1.0f));
        b2Body *neck1body = m_world->CreateBody(&bodyDef);
        neck1body->CreateFixture(&neckShape, 5.0f);
        neck1->SetBody(neck1body);
        neck1->SetTexture(neckTex);
        neck1->SetFlameTexture(flameTexture);
        neck1->SetFlameGlowTexture(flameGlowTexture);
        neck1->SetBubbleTexture(bubbleTexture);
        neck1->SetOily();

        neckJoint.bodyA = neck0body;
        neckJoint.bodyB = neck1body;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        b2RevoluteJoint* neck0j = (b2RevoluteJoint*)m_world->CreateJoint(&neckJoint);

        GameObject *neck2 = CreateObject(neck1);
        bodyDef.position = ToB2(position + vec2f(1.15f, 1.15f));
        b2Body *neck2body = m_world->CreateBody(&bodyDef);
        neck2body->CreateFixture(&neckShape, 5.0f);
        neck2->SetBody(neck2body);
        neck2->SetTexture(neckTex);
        neck2->SetFlameTexture(flameTexture);
        neck2->SetFlameGlowTexture(flameGlowTexture);
        neck2->SetBubbleTexture(bubbleTexture);
        neck2->SetOily();

        neckJoint.bodyA = neck1body;
        neckJoint.bodyB = neck2body;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-neckJlen, 0.0f);
        b2RevoluteJoint* neck1j = (b2RevoluteJoint*)m_world->CreateJoint(&neckJoint);

        neckJoint.bodyA = neck2body;
        neckJoint.bodyB = headBody;
        neckJoint.localAnchorA.Set(neckJlen, 0.0f);
        neckJoint.localAnchorB.Set(-0.4f, -0.4f);
        b2RevoluteJoint* neck2j = (b2RevoluteJoint*)m_world->CreateJoint(&neckJoint);

        // Legs
        {
            const TextureHandle legTex = GetCache().GetTexture("bird_leg.tex");
            b2BodyDef legDef;
            legDef.type = b2_dynamicBody;
            legDef.position = ToB2(position - vec2f(0.5f, 0.5f));
            b2PolygonShape legShape;
            legShape.SetAsBox(0.25f, 0.5f);

            b2Body *legBody = m_world->CreateBody(&legDef);
            legBody->CreateFixture(&legShape, 1.0f);

            GameObject *leg = CreateObject(neck2);
            leg->SetBody(legBody);
            leg->SetTexture(legTex);
            leg->SetFlameTexture(flameTexture);
            leg->SetFlameGlowTexture(flameGlowTexture);
            leg->SetOily();
//            leg->SetNext(object);

            b2RevoluteJointDef hipDef;
            hipDef.bodyA = body;
            hipDef.bodyB = legBody;
            hipDef.localAnchorA.Set(-0.5f, -0.5f);
            hipDef.localAnchorB.Set(0.0f, 0.5f);
            hipDef.collideConnected = false;
            hipDef.enableLimit = true;
            hipDef.lowerAngle = -90.0f * rob::DEG2RAD_f;
            hipDef.upperAngle = 0.0f * rob::DEG2RAD_f;
            m_world->CreateJoint(&hipDef);


            legDef.position = ToB2(position + vec2f(0.5f, -0.5f));
            legBody = m_world->CreateBody(&legDef);
            legBody->CreateFixture(&legShape, 1.0f);

            leg = CreateObject(leg);
            leg->SetBody(legBody);
            leg->SetTexture(legTex);
            leg->SetFlameTexture(flameTexture);
            leg->SetFlameGlowTexture(flameGlowTexture);
            leg->SetOily();
            leg->SetNext(bird);

            hipDef.bodyA = body;
            hipDef.bodyB = legBody;
            hipDef.localAnchorA.Set(0.5f, -0.5f);
            hipDef.localAnchorB.Set(0.0f, 0.5f);
            hipDef.collideConnected = false;
            hipDef.enableLimit = true;
            hipDef.lowerAngle = 0.0f * rob::DEG2RAD_f;
            hipDef.upperAngle = 90.0f * rob::DEG2RAD_f;
            m_world->CreateJoint(&hipDef);
        }

        BirdLogic *logic = new BirdLogic(headBody, neckj, neck0j, neck1j, neck2j);
        bird->SetLogic(logic);
        // To update the color of the bird according to the oilyness factor
        logic->Update(0.0f);

        return bird;
    }

    void DuckState::CreateOven()
    {
        b2BodyDef def;
        def.type = b2_staticBody;
        def.position = b2Vec2(PLAY_AREA_RIGHT / 2.0f, PLAY_AREA_BOTTOM - 2.0f);
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
        if (m_inUpdate)
            object->SetDestroyed(true);
        else
            DestroySingleObject(object);
    }

    void DuckState::DestroyObjectList(GameObject *object, GameObject *last)
    {
        GameObject *next = object->GetNext();
        DestroyObject(object);
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

                if (m_mouseJoint && object->GetBody() == m_mouseJoint->GetBodyB())
                {
                    DestroyMouseJoint();
                }

                m_world->DestroyBody(object->GetBody());
                m_objectPool.Return(object);
                return;
            }
        }
        ROB_ASSERT(0);
    }

    void DuckState::DestroyLinkedObjects(GameObject *object)
    {
        GameObject *next = object->GetNext();
        DestroyObject(object);
        if (next) DestroyObjectList(next, object);
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

    void DuckState::BirdBurned(GameObject *bird)
    {
//        if (!IsGameOver())
//        {
//            m_gameData.m_birdsKilled++;
//        }

        //rob::log::Info("Bird burned");
        m_sounds.PlayBurningSound(bird->GetPosition());

        BirdDied(bird);
    }

    void DuckState::BirdSaved(GameObject *bird)
    {
        if (!IsGameOver())
        {
            float oil = bird->GetOilyness();

            if (oil <= 0.5f)
            {
                //rob::log::Info("Bird saved");
                m_sounds.PlaySavedSound(bird->GetPosition());
                m_sounds.PlayScoreSound(vec2f(PLAY_AREA_LEFT, PLAY_AREA_TOP));

                m_gameData.m_birdsSaved++;
                float invOil = 1.0f - oil;
                int points = (invOil * invOil * 100.0f);
                m_gameData.m_score += points;

                m_scoreTimer = SCORE_TIME;
                m_lastPoints = points;
            }
            else
            {
                BirdDied(bird);
            }
        }
    }

    void DuckState::BirdDied(GameObject *bird)
    {
        if (!IsGameOver())
        {
            m_gameData.m_birdsKilled++;
        }

        //rob::log::Info("Bird died");
        m_sounds.PlayDyingSound(bird->GetPosition());
    }

    void DuckState::BirdEnteredWater(GameObject *bird)
    {
        m_sounds.PlayWaterSound(bird->GetPosition());
    }

    void DuckState::BirdExitedWater(GameObject *bird)
    {
        m_sounds.PlayWaterSound(bird->GetPosition());
    }

    bool DuckState::IsGameOver() const
    {
        return (MAX_LIVES - m_gameData.m_birdsKilled) <= 0;
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
    }

    void DuckState::RealtimeUpdate(const Time_t deltaMicroseconds)
    {
        const float deltaTime = float(deltaMicroseconds) / 1e6f;
        m_fadeEffect.Update(deltaTime);
    }

    void DuckState::NewBird()
    {
        if (m_spawnSensor.CanSpawn())
            CreateBird(vec2f(PLAY_AREA_LEFT * 2.0f, PLAY_AREA_BOTTOM + 4.0f));
    }

    void DuckState::CreateWaste()
    {
        b2CircleShape shape;
        shape.m_radius = 2.0f;

        b2ParticleGroupDef groupDef;
        groupDef.shape = &shape;
        groupDef.color.Set(32, 25, 16, 255);
        groupDef.position.Set(0.0f, 2.0f);
        groupDef.flags = b2_viscousParticle | b2_fixtureContactListenerParticle;
        groupDef.lifetime = 100.0f;
        m_waste->CreateParticleGroup(groupDef);

//        b2ParticleDef def;
//        def.color.Set(25, 20, 12, 255);
//        def.position.Set(0.0f, 0.0f);
//        def.flags = b2_waterParticle;
//        def.group = 0;
//        m_waste->CreateParticle(def);
    }

    void DuckState::DestroyMouseJoint()
    {
        if (m_mouseJoint)
        {
            m_world->DestroyJoint(m_mouseJoint);
            m_mouseJoint = nullptr;
        }
    }

    void DuckState::Update(const GameTime &gameTime)
    {
        const float deltaTime = gameTime.GetDeltaSeconds();

        static float birdTimer = 0.0f;
        static float birdTimerAdd = 10.0f;
        birdTimer -= deltaTime;
        if (birdTimer < 0.0f)
        {
            NewBird();
            birdTimer += birdTimerAdd;
            rob::log::Info(birdTimerAdd, " ", gameTime.GetTotalSeconds());
        }
        birdTimerAdd = 10.0f - rob::Log10(gameTime.GetTotalSeconds());

        if (m_scoreTimer > 0.0f)
        {
            m_scoreTimer -= deltaTime;
            if (m_scoreTimer < 0.0f) m_scoreTimer = 0.0f;
        }

        m_sounds.UpdateTime(gameTime);

        m_inUpdate = true;

        if (IsGameOver() && m_mouseJoint)
            DestroyMouseJoint();
        if (IsGameOver())
        {
            m_fadeEffect.Activate(0.5f);
            m_fadeEffect.Update(deltaTime);
        }

        if (m_mouseJoint)
        {
            b2Body *body = m_mouseJoint->GetBodyB();
            const float bodyAngle = body->GetAngle();
            const float nextAngle = bodyAngle + body->GetAngularVelocity() * deltaTime;
            const float totalRotation = m_originalAngle - nextAngle;
            body->ApplyTorque(totalRotation * body->GetMass() * 10.0f, true);
        }

        m_world->Step(deltaTime, 8, 8, 1);

        {
            const b2ParticleBodyContact *contacts = m_waste->GetBodyContacts();
            for (int i = 0; i < m_waste->GetBodyContactCount(); i++)
            {
                const b2ParticleBodyContact &contact = contacts[i];
//                if (m_killSensor.GetBody() == contact.body)
//                    m_waste->DestroyParticle(contact.index);
//                if (!contact.fixture || contact.fixture->GetFilterData().categoryBits != SensorBits) continue;
                if (contact.fixture->GetFilterData().categoryBits != SensorBits) continue;
                log::Debug("LOOLOLOLOL");
                Sensor *sensor = (Sensor*)contact.fixture->GetUserData();
                if (sensor)// && sensor->WithParticles())
                {
                    sensor->BeginParticleContact(m_waste, &contact);
                }
            }
        }

        size_t deadCount = 0;
        GameObject *dead[MAX_OBJECTS];

        for (size_t i = 0; i < m_objectCount; i++)
        {
            if (!m_objects[i]->IsDestroyed())
            {
//                if (m_objects[i]->IsSaved())
//                {
//                    log::Info("Bird saved");
//                    DestroyLinkedObjects(m_objects[i]);
//                }
//                else
//                {
                    m_objects[i]->Update(gameTime);
//                }
            }

            if (m_objects[i]->IsDestroyed())
                dead[deadCount++] = m_objects[i];
        }
        for (size_t i = 0; i < deadCount; i++)
        {
            DestroySingleObject(dead[i]);
        }

        m_firedColor = gameTime.GetTotalSeconds() * 8.0f;

        m_inUpdate = false;
    }

    void DuckState::RenderGameOver()
    {
        Renderer &renderer = GetRenderer();
        renderer.SetColor(Color(1.0f, 1.0f, 1.0f));
        renderer.BindFontShader();

        const Viewport vp = renderer.GetView().m_viewport;

        TextLayout layout(renderer, vp.w / 2.0f, vp.h / 3.0f);

        renderer.SetFontScale(4.0f);
//        layout.AddTextAlignC("Game over", 0.0f);
//        layout.AddLine();
//        renderer.SetFontScale(2.0f);

//        int r = 128 + rand() % 128;
//        int g = rand() % 255; // TODO: use rob/math/Random
//        float rr = r / 255.0f;
//        float gg = g / 255.0f;

        float rr = 1.1f + FastSin(m_firedColor) * 0.25f;
        float gg = 0.5f + FastCos(m_firedColor*2.5f) * 0.5f;
        float rg = 0.25f + FastCos(m_firedColor*6.45f) * 0.25f;

        renderer.SetColor(Color(rr + rg, gg * rr + rg, gg * rr * 0.2f));
        layout.AddTextAlignC("You are FIRED!", 0.0f);
        layout.AddLine();

        renderer.SetColor(Color(1.0f, 1.0f, 1.0f));
        renderer.SetFontScale(1.0f);
        layout.AddTextAlignC("Press [space] to continue", 0.0f);
    }

    void DuckState::Render()
    {
        Renderer &renderer = GetRenderer();
        renderer.SetView(m_view);
        renderer.SetModel(mat4f::Identity);

//        renderer.SetColor(Color(0.18f, 0.14f, 0.14f));
        renderer.SetColor(Color(60 / 255.0f, 71 / 255.0f, 49 / 255.0f));
        renderer.BindColorShader();
        renderer.DrawFilledRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);

        renderer.SetModel(mat4f::Identity);
        renderer.GetGraphics()->BindTexture(0, GetCache().GetTexture("bg.tex"));
        renderer.GetGraphics()->SetUniform(renderer.GetGlobals().texture0, 0);
        renderer.SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        renderer.BindTextureShader();
        renderer.DrawTexturedRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);
//        renderer.SetColor(Color(1.0f, 1.0f, 1.0f, 0.8f));
//        renderer.GetGraphics()->SetBlendAdditive();
//        renderer.GetGraphics()->BindTexture(0, GetCache().GetTexture("oven_light.tex"));
//        renderer.DrawTexturedRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);
//        renderer.GetGraphics()->SetBlendAlpha();

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

        renderer.SetModel(mat4f::Identity);
        renderer.BindColorShader();

        const b2Vec2 *positions = m_waste->GetPositionBuffer();
        const b2ParticleColor *colors = m_waste->GetColorBuffer();
        const float radius = m_waste->GetRadius();
        for (int i = 0; i < m_waste->GetParticleCount(); i++)
        {
            const b2Color col = colors[i].GetColor();
            renderer.SetColor(Color(col.r, col.g, col.b, colors[i].a / 255.0f));
            renderer.DrawFilledCircle(positions[i].x, positions[i].y, radius);
        }

        float ovenLightAlpha = 0.8f + FastSin(m_firedColor) * 0.15f + FastCos(m_firedColor + 0.26f) * 0.05f;

        renderer.SetModel(mat4f::Identity);
        renderer.GetGraphics()->SetUniform(renderer.GetGlobals().texture0, 0);
        renderer.BindTextureShader();
        renderer.GetGraphics()->SetBlendAdditive();
//        renderer.SetColor(Color(1.0f, 1.0f, 1.0f, 0.8f));
        renderer.SetColor(Color(1.0f, 1.0f, 1.0f, ovenLightAlpha));
        renderer.GetGraphics()->BindTexture(0, GetCache().GetTexture("oven_light.tex"));
        renderer.DrawTexturedRectangle(PLAY_AREA_LEFT, PLAY_AREA_BOTTOM, PLAY_AREA_RIGHT, PLAY_AREA_TOP);
        renderer.GetGraphics()->SetBlendAlpha();

        if (m_drawBox2D)
        {
//            renderer.SetModel(mat4f::Identity);
            renderer.BindColorShader();
            m_world->DrawDebugData();
        }

//        renderer.SetModel(mat4f::Identity);
        m_fadeEffect.Render(&renderer);

        renderer.SetView(GetDefaultView());
        renderer.BindFontShader();
        renderer.SetColor(Color::White);

        char text[40];

        const float f = m_scoreTimer / SCORE_TIME;
        const float k = 1.0f - f;

        TextLayout layout(renderer, 0.0f, 0.0f);
        renderer.SetFontScale(1.0f);

        StringPrintF(text, "Score: %i", m_gameData.m_score);
        renderer.SetColor(Color(k, 1.0f, k));
        layout.AddTextAlignL(text, 0.0f);
        layout.AddLine();

        StringPrintF(text, "Birds saved: %i", m_gameData.m_birdsSaved);
        renderer.SetColor(Color::White);
        layout.AddTextAlignL(text, 0.0f);
        layout.AddLine();

        // Draw bird heads
        {
            const TextureHandle birdHead = GetCache().GetTexture("bird_head.tex");
            renderer.BindTextureShader();
            renderer.GetGraphics()->BindTexture(0, birdHead);
            float bhX = 10.0f;
            const float bhY = 60.0f;
            const float bhS = 40.0f;
            const int lives = MAX_LIVES - m_gameData.m_birdsKilled;
            for (int i = 0; i < lives; i++)
            {
                renderer.DrawTexturedRectangle(bhX, bhY + bhS, bhX + bhS, bhY);
                bhX += bhS;
            }
        }

        if (m_scoreTimer > 0.0f)
        {
            StringPrintF(text, "%i", m_lastPoints);
            renderer.BindFontShader();
            renderer.SetColor(Color(k, 1.0f, k, f));
            float x0 = 200.0f;
            float y0 = 0.0f;
            float x1 = 100.0f;
            float y1 = 0.0f;
            renderer.DrawText(x0*f + x1*k, y0*f + y1*k, text);
        }

        if (IsGameOver())
            RenderGameOver();
    }

    void DuckState::OnKeyPress(Keyboard::Key key, Keyboard::Scancode scancode, uint32_t mods)
    {
        switch (key)
        {
            case Keyboard::Key::P:
            {
                if (IsGameOver()) break;
                if (m_time.IsPaused())
                {
                    m_time.Resume();
                    m_fadeEffect.Reset();
                }
                else
                {
                    m_time.Pause();
                    m_fadeEffect.Activate(1.0f);
                }
                break;
            }
            case Keyboard::Key::Space:
            {
                if (IsGameOver())
                    ChangeState(STATE_HighScore);
                break;
            }
            case Keyboard::Key::Escape:
            {
                ChangeState(STATE_MainMenu);
                break;
            }
            default: break;
        }

        // TODO: These are for debugging
        {
            if (key == Keyboard::Key::Tab)
                m_drawBox2D = !m_drawBox2D;
            if (key == Keyboard::Key::B)
                NewBird();
            if (key == Keyboard::Key::W)
                CreateWaste();
            if (key == Keyboard::Key::Space && !IsGameOver())
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

        if (IsGameOver())
            return;

        // Make a small box.
        b2AABB aabb;
        b2Vec2 d(0.001f, 0.001f);
        b2Vec2 p = ToB2(m_mouseWorld);
        aabb.lowerBound = p - d;
        aabb.upperBound = p + d;

        // Query the world for overlapping shapes.
        QueryCallback callback(p);
        m_world->QueryAABB(&callback, aabb);

        if (callback.m_fixture)
        {
            bool wash = !(button == MouseButton::Left);
            const float force = wash ? 20.0f : 1000.0f;

            b2Body* body = callback.m_fixture->GetBody();
            b2MouseJointDef md;
            md.bodyA = m_worldBody;
            md.bodyB = body;
            md.target = p;
            md.maxForce = force * body->GetMass();
            md.dampingRatio = 0.98f * force / 1000.0f; // TODO: not sure if this does anything good.
            m_mouseJoint = (b2MouseJoint*)m_world->CreateJoint(&md);
            body->SetAwake(true);

            m_originalAngle = body->GetAngle();

            if (wash && body->GetUserData())
            {
                GameObject *bird = (GameObject*)body->GetUserData();
                bird->Wash();
            }
        }
    }

    void DuckState::OnMouseUp(MouseButton button, int x, int y)
    {
        DestroyMouseJoint();
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
        }
    }

} // duck
