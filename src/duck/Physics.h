
#ifndef H_DUCK_PHYSICS_H
#define H_DUCK_PHYSICS_H

#include "rob/math/Math.h"
#include "Box2D/Box2D.h"

namespace duck
{

    using rob::vec2f;

    inline vec2f FromB2(const b2Vec2 &v)
    { return vec2f(v.x, v.y); }

    inline b2Vec2 ToB2(const vec2f &v)
    { return b2Vec2(v.x, v.y); }

} // duck

#endif // H_DUCK_PHYSICS_H

