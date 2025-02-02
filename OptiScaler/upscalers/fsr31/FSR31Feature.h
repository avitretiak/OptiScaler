#pragma once

#include <pch.h>

#include <upscalers/IFeature.h>

#include <ffx_api.h>
#include <ffx_upscale.h>
#include <include/detours/detours.h>

inline static std::string ResultToString(ffxReturnCode_t result)
{
    switch (result)
    {
        case FFX_API_RETURN_OK: return "The oparation was successful.";
        case FFX_API_RETURN_ERROR: return "An error occurred that is not further specified.";
        case FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE: return "The structure type given was not recognized for the function or context with which it was used. This is likely a programming error.";
        case FFX_API_RETURN_ERROR_RUNTIME_ERROR: return "The underlying runtime (e.g. D3D12, Vulkan) or effect returned an error code.";
        case FFX_API_RETURN_NO_PROVIDER: return "No provider was found for the given structure type. This is likely a programming error.";
        case FFX_API_RETURN_ERROR_MEMORY: return "A memory allocation failed.";
        case FFX_API_RETURN_ERROR_PARAMETER: return "A parameter was invalid, e.g. a null pointer, empty resource or out-of-bounds enum value.";
        default: return "Unknown";
    }
}

inline static void FfxLogCallback(uint32_t type, const wchar_t* message)
{
    std::wstring string(message);

    //if (type == FFX_API_MESSAGE_TYPE_ERROR)
    //	LOG_ERROR("FSR Runtime: {0}", str);
    //else if (type == FFX_API_MESSAGE_TYPE_WARNING)
    //	LOG_WARN("FSR Runtime: {0}", str);
    //else
    LOG_DEBUG("FSR Runtime: {0}", wstring_to_string(string));
}

class FSR31Feature : public virtual IFeature
{
private:
    double _lastFrameTime;
    bool _depthInverted = false;
    unsigned int _lastWidth = 0;
    unsigned int _lastHeight = 0;
    static inline feature_version _version{ 3, 1, 1 };

protected:
    std::string _name = "FSR 3.X";

    ffxContext _context = nullptr;
    ffxCreateContextDescUpscale _contextDesc = {};

    virtual bool InitFSR3(const NVSDK_NGX_Parameter* InParameters) = 0;

    double MillisecondsNow();
    double GetDeltaTime();
    void SetDepthInverted(bool InValue);

    static inline void parse_version(const char* version_str) 
    {
        if (sscanf_s(version_str, "%u.%u.%u", &_version.major, &_version.minor, &_version.patch) != 3)
            LOG_WARN("can't parse {0}", version_str);
    }

    float _velocity = 0.0f;

public:
    bool IsDepthInverted() const;
    feature_version Version() final { return _version; }
    const char* Name() override { return _name.c_str(); }

    FSR31Feature(unsigned int InHandleId, NVSDK_NGX_Parameter* InParameters) : IFeature(InHandleId, InParameters)
    {
        _lastFrameTime = MillisecondsNow();
    }

    ~FSR31Feature();
};