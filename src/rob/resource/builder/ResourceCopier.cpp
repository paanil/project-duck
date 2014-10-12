
#include "ResourceCopier.h"
#include "../../filesystem/FileCopy.h"

namespace rob
{

    ResourceCopier::ResourceCopier()
    {
        m_extensions.push_back(".ion");
        m_extensions.push_back(".wav");
        m_extensions.push_back(".fnt");
    }

    bool ResourceCopier::Build(const std::string &directory, const std::string &filename,
                               const std::string &destDirectory, const std::string &destFilename)
    {
        FileCopy(filename.c_str(), destFilename.c_str());
        return true;
    }

} // rob
