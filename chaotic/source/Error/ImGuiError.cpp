#include "Error/ChaoticError.h"
#include <cstdio>

// Create the imgui error class and funnel IM_ASSERT through CHAOTIC_FATAL
class ImGuiError : public ChaoticError
{
public:
    // Single message constructor, the message is stored in the info.
    ImGuiError(int line, const char* file, const char* msg) noexcept
        : ChaoticError(line, file)
    {
        // Print info error formatted string.
        snprintf(info, 2048, "\n[Assertion Failed]\n%s%s", msg, origin);
    }

    // ImGui Error type override.
    const char* GetType() const noexcept override { return "ImGui Error"; }
};

[[noreturn]] void handleImGuiAssertion(int line, const char* file, const char* msg) noexcept
{
    CHAOTIC_FATAL(ImGuiError(line, file, msg));
}