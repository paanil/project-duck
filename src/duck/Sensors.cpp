
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

} // duck
