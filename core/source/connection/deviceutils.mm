#include <AVFoundation/AVCaptureDevice.h>
#include <vector>
#include <string>

namespace core
{
namespace connection
{
std::vector<std::string> getCameraNames()
{
    std::vector<std::string> cameraNames;
    @autoreleasepool {
        AVCaptureDeviceDiscoverySession *discoverySession =
            [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeExternal]
                                                                   mediaType:AVMediaTypeVideo
                                                                    position:AVCaptureDevicePositionUnspecified];

        NSArray<AVCaptureDevice *> *devices = discoverySession.devices;
        devices = [devices sortedArrayUsingComparator:^NSComparisonResult(AVCaptureDevice *d1, AVCaptureDevice *d2){ return [d1.uniqueID compare:d2.uniqueID]; }];

        for (AVCaptureDevice* device in devices)
        {
          cameraNames.push_back(device.localizedName.UTF8String);
        }
    }
    return cameraNames;
}
} // namespace connection
} // namespace core
