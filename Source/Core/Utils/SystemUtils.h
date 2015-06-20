#ifndef IU_CORE_SYSTEMUTILS_H
#define IU_CORE_SYSTEMUTILS_H

#pragma once

#include <string>

namespace IuCoreUtils
{
    std::string getOsName();
    std::string getOsVersion();
    std::string getCpuFeatures();
    bool isOs64Bit();
}

#endif
