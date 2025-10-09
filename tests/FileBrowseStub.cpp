#include <string>

// Stub implementation of browseForFile for testing
// This avoids linking platform-specific file dialog code in tests
namespace cabbage {
namespace File {
    std::string browseForFile(const std::string& title, const std::string& initialDir, const std::string& filters) {
        // Return a dummy path for testing
        return "/test/dummy/path.wav";
    }
}
}