
#include "GameObject.h"
#include "rob/renderer/Renderer.h"

namespace duck
{

    GameObject::GameObject()
        : m_body(nullptr)
    { }

    void GameObject::SetPosition(const vec2f &pos)
    { m_body->SetTransform(ToB2(pos), m_body->GetAngle()); }

    vec2f GameObject::GetPosition() const
    { return FromB2(m_body->GetPosition()); }

    vec2f GameObject::GetDimensions() const
    {
        const b2Fixture *fixture = m_body->GetFixtureList();
        const b2Shape *shape = fixture->GetShape();

        b2AABB aabb;
        b2Transform transform;
        transform.SetIdentity();
        shape->ComputeAABB(&aabb, transform, 0);

        return FromB2(aabb.GetExtents());
    }

    void GameObject::Update(const GameTime &gameTime)
    {

    }

    void GameObject::Render(rob::Renderer *renderer)
    {
        const vec2f pos = GetPosition();
        const vec2f dim = GetDimensions();

        renderer->SetColor(rob::Color::White);
        renderer->BindColorShader();

        const b2Fixture *fixture = m_body->GetFixtureList();
        const b2Shape *shape = fixture->GetShape();

        switch (shape->GetType())
        {
        case b2Shape::e_polygon:
            renderer->DrawFilledRectangle(pos.x - dim.x, pos.y - dim.y, pos.x + dim.x, pos.y + dim.y);
            break;

        case b2Shape::e_circle:
            renderer->DrawFilledCircle(pos.x, pos.y, dim.x);
            break;

        default:
            break;
        }
    }

} // duck
