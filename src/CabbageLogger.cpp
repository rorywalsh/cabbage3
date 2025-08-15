#include "CabbageLogger.h"

namespace cabbage {

// Static member definitions
FileLogger* FileLogger::instance = nullptr;
std::mutex FileLogger::instanceMutex;

} // namespace cabbage
