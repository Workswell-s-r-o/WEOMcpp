#include "core/connection/deviceutils.h"
#include "core/utils.h"

#ifdef __linux__
extern "C" {
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <unistd.h>
}
#endif

namespace core
{
namespace connection
{
#ifdef __APPLE__
std::vector<std::string> getCameraNames();
#endif

std::pair<std::string, std::string> findVideoDeviceNameWithFormat(const std::string& serialNumber)
{
#ifdef __linux__
    v4l2_capability cap;
    for (int i = 0; i < 64; ++i)
    {
        const auto devName = utils::format("/dev/video{}", i);
        auto fd = open(devName.c_str(), O_RDONLY);
        if (fd == -1) { continue; }
        v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) >= 0)
        {
            if (std::string(reinterpret_cast<const char*>(cap.card), 32).find(serialNumber) != std::string::npos)
            {
                return {devName, "v4l2"};
            }
        }
        close(fd);
    }
    return {"", "v4l2"};
#endif
#ifdef __APPLE__
    const auto cameraNames =  getCameraNames();
    for(std::size_t i = 0; i < cameraNames.size(); ++i)
    {
        if (cameraNames[i].find(serialNumber) != std::string::npos)
        {
            return {utils::format("{}", i), "avfoundation"};
        }
    }
    return {std::string(""), "avfoundation"};
#else
    return {utils::format("video=WEOM {}", serialNumber), "dshow"};
#endif
}

} // namespace connection
} // namespace core
