
#include "Sensors.h"
#include "DuckState.h"

namespace duck
{

    void OvenSensor::BeginContact(void *userData)
    {
        GameObject *firstPart = (GameObject*)userData;

        if (firstPart->IsBurned())
            return;

        GameObject *part = firstPart;

        do
        {
            part->SetBurned();
            part->SetColor(Color(0.0f, 0.0f, 0.0f, 1.0f));
            part = part->GetNext();
        } while (part != firstPart);

        m_duckState->BirdGotBurned(firstPart);
    }

    KillSensor::KillSensor()
        : Sensor(0xFFFF)
    { }

    void KillSensor::BeginContact(void *userData)
    {
        GameObject *obj = (GameObject*)userData;
        if (obj) obj->SetAlive(false);
    }

    WaterSensor::WaterSensor()
        : Sensor(BirdBits)
    { }

    void WaterSensor::BeginContact(void* userData)
    {
        GameObject *firstPart = (GameObject*)userData;
        GameObject *part = firstPart;
        do
        {
            part->SetInWater(true);
            part = part->GetNext();
        } while (part != firstPart);
    }

    void WaterSensor::EndContact(void* userData)
    {
        GameObject *firstPart = (GameObject*)userData;
        GameObject *part = firstPart;
        do
        {
            part->SetInWater(false);
            part = part->GetNext();
        } while (part != firstPart);
    }

} // duck
