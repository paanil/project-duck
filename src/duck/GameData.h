
#ifndef H_DUCK_GAME_DATA_H
#define H_DUCK_GAME_DATA_H

#include "HighScoreList.h"

namespace duck
{

    struct GameData
    {
        int             m_birdsKilled;
        int             m_birdsSaved;
        int             m_score;
        HighScoreList   m_highScores;
    };

} // duck

#endif // H_DUCK_GAME_DATA_H

