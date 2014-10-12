
#ifndef H_ROB_FILE_STAT_H
#define H_ROB_FILE_STAT_H

#include <ctime>

namespace rob
{

    bool FileExists(const char * const path);
    bool IsDirectory(const char * const path);

    time_t GetModifyTime(const char * const path);

} // rob

#endif // H_ROB_FILE_STAT_H

