
#ifndef H_DUCK_LOGIC_H
#define H_DUCK_LOGIC_H

#include "PidController.h"
#include "rob/Log.h"

namespace duck
{

    class GameObject;

    class Logic
    {
    public:
        virtual ~Logic() { }

        virtual void Update(float deltaTime) = 0;
    };

    class BirdLogic : public Logic
    {
    public:
        BirdLogic(b2Body *body, b2RevoluteJoint *joint0, b2RevoluteJoint *joint1, b2RevoluteJoint *joint2)
            : m_neck0Controller()
            , m_neck1Controller()
            , m_neck2Controller()
            , m_targetAngle(0.0f) //30.0f * rob::DEG2RAD) //-90.0f * rob::DEG2RAD)
            , m_headBody(body)
            , m_neck0Joint(joint0)
            , m_neck1Joint(joint1)
            , m_neck2Joint(joint2)
            , m_targetPos()
        {
            m_neck0Controller.SetGains(0.5f, 0.5f, 0.5f);
        }

        void Update(float deltaTime) override
        {
//            float err = m_targetAngle - m_headBody->GetAngle();
//            while (err > rob::PI_f) err = (rob::PI_f - err);
//            while (err < -rob::PI_f) err = -(err + rob::PI_f);
            {
//                const b2Vec2 rr = m_neck0Joint->GetAnchorB() - m_neck0Joint->GetAnchorA();
//                const b2Vec2 R(-rr.y, rr.x);
//                const b2Vec2 F = ToB2(m_targetPos) - m_neck0Joint->GetAnchorA();
//                const float angle = b2Dot(R, F);

//                m_neck0Controller.SetError(m_targetAngle - m_headBody->GetAngle());
//                m_neck0Controller.SetError(angle - m_headBody->GetAngle());
                b2Body *body = m_neck0Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
//                float err = m_targetAngle - m_neck0Joint->GetBodyB()->GetAngle();
            while (err > rob::PI_f) err = (rob::PI_f - err);
            while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck0Controller.SetError(err);
                m_neck0Controller.Step(deltaTime);
                const float targetSpeed = m_neck0Controller.GetOutput();
                m_neck0Joint->SetMotorSpeed(targetSpeed);
            }
            {
                b2Body *body = m_neck1Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
//                float err = m_targetAngle - m_neck1Joint->GetBodyB()->GetAngle();
            while (err > rob::PI_f) err = (rob::PI_f - err);
            while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck1Controller.SetError(err);
                m_neck1Controller.Step(deltaTime);
                const float targetSpeed = m_neck1Controller.GetOutput();
                m_neck1Joint->SetMotorSpeed(targetSpeed);
            }
            {
                b2Body *body = m_neck2Joint->GetBodyB();
                float angle = body->GetAngle();
                angle = fmodf(angle, 2.0f * rob::PI_f);
                body->SetTransform(body->GetPosition(), angle);
                float err = m_targetAngle - angle;
//                float err = m_targetAngle - m_neck2Joint->GetBodyB()->GetAngle();
            while (err > rob::PI_f) err = (rob::PI_f - err);
            while (err < -rob::PI_f) err = -(err + rob::PI_f);
                m_neck2Controller.SetError(err);
                m_neck2Controller.Step(deltaTime);
                const float targetSpeed = m_neck2Controller.GetOutput();
                m_neck2Joint->SetMotorSpeed(targetSpeed);
            }
        }
    private:
        PidController m_neck0Controller;
        PidController m_neck1Controller;
        PidController m_neck2Controller;
        float m_targetAngle;
        b2Body *m_headBody;
        b2RevoluteJoint *m_neck0Joint;
        b2RevoluteJoint *m_neck1Joint;
        b2RevoluteJoint *m_neck2Joint;
        vec2f m_targetPos;
    };

} // duck

#endif // H_DUCK_LOGIC_H

