
#include "GameObject.h"
#include "Logic.h"
#include "rob/renderer/Renderer.h"
#include "rob/graphics/Graphics.h"

namespace duck
{

    using namespace rob;

    GameObject::GameObject()
        : m_body(nullptr)
        , m_logic(nullptr)
        , m_color(Color::White)
        , m_texture(InvalidHandle)
        , m_flameTexture(InvalidHandle)
        , m_renderLayer(0)
        , m_burnTimer(0.0f)
        , m_destroyed(false)
        , m_saved(false)
        , m_burned(false)
        , m_inWater(false)
        , m_next(nullptr)
    { }

    GameObject::~GameObject()
    {
        delete m_logic;
    }

    void GameObject::SetPosition(const vec2f &pos)
    { m_body->SetTransform(ToB2(pos), m_body->GetAngle()); }

    vec2f GameObject::GetPosition() const
    { return FromB2(m_body->GetPosition()); }

    void GameObject::SetLogic(Logic *logic)
    { m_logic = logic; m_logic->SetOwner(this); }

    Logic* GameObject::GetLogic()
    { return m_logic; }

    vec2f GameObject::GetDimensions() const
    {
        const b2Fixture *fixture = m_body->GetFixtureList();
        const b2Shape *shape = fixture->GetShape();

        b2AABB aabb;
        b2Transform tr;
        tr.SetIdentity();

        shape->ComputeAABB(&aabb, tr, 0);
        while (fixture)
        {
            const b2Shape *shape = fixture->GetShape();
            for (int i = 0; i < shape->GetChildCount(); i++)
            {
                const b2Vec2 r(shape->m_radius, shape->m_radius);
                b2AABB shapeAabb;
                shape->ComputeAABB(&shapeAabb, tr, i);
                shapeAabb.lowerBound = shapeAabb.lowerBound + r;
                shapeAabb.upperBound = shapeAabb.upperBound - r;
                aabb.Combine(shapeAabb);
            }
            fixture = fixture->GetNext();
        }

        return FromB2(aabb.GetExtents());
    }

    void GameObject::SetColor(const Color &color)
    { m_color = color; }

    Color GameObject::GetColor() const
    { return m_color; }

    void GameObject::SetTexture(TextureHandle texture)
    { m_texture = texture; }

    TextureHandle GameObject::GetTexture() const
    { return m_texture; }

    void GameObject::SetLayer(int layer)
    { m_renderLayer = layer; }

    int GameObject::GetLayer() const
    { return m_renderLayer; }

    void GameObject::Update(const GameTime &gameTime)
    {
        if (m_logic) m_logic->Update(gameTime.GetDeltaSeconds());
        if (m_burnTimer > 0.0f)
            m_burnTimer -= gameTime.GetDeltaSeconds();
        if (m_inWater && !m_burned)
        {
            float dt = gameTime.GetDeltaSeconds();
            float speed = 0.1f;
            m_color.r += speed * dt;
            m_color.g += speed * dt;
            m_color.b += speed * dt;
            if (m_color.r > 1.0f) m_color.r = 1.0f;
            if (m_color.g > 1.0f) m_color.g = 1.0f;
            if (m_color.b > 1.0f) m_color.b = 1.0f;
        }
    }

    void GameObject::Render(Renderer *renderer)
    {
        const vec2f dim = GetDimensions();

//        renderer->SetColor(rob::Color::White);
        renderer->SetColor(m_color);

        const b2Fixture *fixture = m_body->GetFixtureList();
        const b2Shape *shape = fixture->GetShape();

        const mat4f model = FromB2Transform(m_body->GetTransform());
        renderer->SetModel(model);

        if (m_texture == InvalidHandle)
        {
            renderer->BindColorShader();
            switch (shape->GetType())
            {
            case b2Shape::e_polygon:
                renderer->DrawFilledRectangle(-dim.x, -dim.y, dim.x, dim.y);
                break;

            case b2Shape::e_circle:
                renderer->DrawFilledCircle(0.0f, 0.0f, dim.x);
                break;

            default:
                break;
            }
        }
        else
        {
            renderer->GetGraphics()->SetUniform(renderer->GetGlobals().texture0, 1);
            renderer->GetGraphics()->BindTexture(1, m_texture);
            renderer->BindTextureShader();
            renderer->DrawTexturedRectangle(-dim.x, -dim.y, dim.x, dim.y);
            if (m_burnTimer > 0.0f)
            {
                int i = m_burnTimer / 0.1f;
                float f = (i & 1) ? -1.0f : 1.0f;
                float k = (m_burnTimer / 3.5f);
                float s = 1.0f + k*0.5f;
                vec2f pos = GetPosition();
                mat4f mat;
                mat.SetTranslation(pos.x, pos.y, 0.0f);
                renderer->SetModel(mat);
                renderer->SetColor(Color(1.0f, 1.0f, 1.0f, k * 2.0f));
                renderer->GetGraphics()->BindTexture(1, m_flameTexture);
                renderer->DrawTexturedRectangle(-dim.x*s*f, -dim.y*s, dim.x*s*f, dim.y*1.5f*s);
            }
//            else if (m_isWashed)
//            {
//                int i = m_burnTimer / 0.1f;
//                float f = (i & 1) ? -1.0f : 1.0f;
//                float k = (m_burnTimer / 3.5f);
//                float s = 1.0f + k*0.5f;
//                vec2f pos = GetPosition();
//                mat4f mat;
//                mat.SetTranslation(pos.x, pos.y, 0.0f);
//                renderer->SetModel(mat);
//                renderer->SetColor(Color(1.0f, 1.0f, 1.0f, k * 2.0f));
//                renderer->GetGraphics()->BindTexture(1, m_bubbleTexture);
//                renderer->DrawTexturedRectangle(-dim.x*s*f, -dim.y*s, dim.x*s*f, dim.y*1.5f*s);
//            }
        }
    }

    void GameObject::SetNext(GameObject *object)
    { m_next = object; }

    GameObject* GameObject::GetNext()
    { return m_next; }

} // duck
