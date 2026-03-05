#include <string>
#include "../src/CabbageUtils.h"

// Stub implementation of cabbage::File::browseForFile for testing.
// File is a class (not a namespace), so the definition must match the
// static method signature declared in CabbageUtils.h.
namespace cabbage {
std::string File::browseForFile(const std::string& /*title*/,
                                const std::string& /*initialDir*/,
                                const std::string& /*filters*/)
{
    return "/test/dummy/path.wav";
}
}