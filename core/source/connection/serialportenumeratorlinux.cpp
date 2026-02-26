#include "core/connection/serialportenumerator.h"
#include <libudev.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream> // For debugging
#include <memory>

namespace core::connection
{

namespace
{
    // RAII wrappers for libudev using unique_ptr with custom deleters
    struct UdevDeleter { void operator()(struct udev* p) const { if (p) udev_unref(p); } };
    struct UdevEnumerateDeleter { void operator()(struct udev_enumerate* p) const { if (p) udev_enumerate_unref(p); } };
    struct UdevDeviceDeleter { void operator()(struct udev_device* p) const { if (p) udev_device_unref(p); } };

    using UdevPtr = std::unique_ptr<struct udev, UdevDeleter>;
    using UdevEnumeratePtr = std::unique_ptr<struct udev_enumerate, UdevEnumerateDeleter>;
    using UdevDevicePtr = std::unique_ptr<struct udev_device, UdevDeviceDeleter>;

    uint16_t hexToUint16(const char* hexStr)
    {
        if (!hexStr)
        {
            return 0;
        }
        return static_cast<uint16_t>(std::stoul(hexStr, nullptr, 16));
    }

    // Function to get a udev device property or sysattr value
    const char* get_udev_property(struct udev_device* dev, const char* key) {
        const char* value = udev_device_get_property_value(dev, key);
        if (!value) {
            value = udev_device_get_sysattr_value(dev, key);
        }
        return value ? value : nullptr;
    }
}

std::vector<core::connection::SerialPortInfo> enumerateSerialPorts()
{
    std::vector<core::connection::SerialPortInfo> ports;

    UdevPtr udev(udev_new());
    if (!udev)
    {
        return ports;
    }

    UdevEnumeratePtr enumerate(udev_enumerate_new(udev.get()));
    if (!enumerate)
    {
        return ports;
    }

    udev_enumerate_add_match_subsystem(enumerate.get(), "tty");
    udev_enumerate_scan_devices(enumerate.get());

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate.get());
    struct udev_list_entry* dev_list_entry;

    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char* path = udev_list_entry_get_name(dev_list_entry);
        UdevDevicePtr dev(udev_device_new_from_syspath(udev.get(), path));
        if (!dev)
        {
            continue;
        }

        const char* devnode = udev_device_get_devnode(dev.get());
        if (devnode)
        {
            std::string devnode_str(devnode);
            // Filter for common USB serial device nodes
            if (devnode_str.rfind("/dev/ttyUSB", 0) == 0 ||
                devnode_str.rfind("/dev/ttyACM", 0) == 0 ||
                devnode_str.rfind("/dev/ttyS", 0) == 0) // Also include standard serial ports
            {
                // usb_device is a borrowed reference, so it remains a raw pointer
                struct udev_device* usb_device = udev_device_get_parent_with_subsystem_devtype(
                    dev.get(), "usb", "usb_device");

                // Fallback for devices like /dev/ttyACM0 which might have an intermediate "usb_interface" parent
                if (!usb_device) {
                    struct udev_device* parent_dev = dev.get();
                    while (parent_dev) {
                        const char* parent_subsystem = udev_device_get_subsystem(parent_dev);
                        const char* parent_devtype = udev_device_get_devtype(parent_dev);

                        if (parent_subsystem && std::string(parent_subsystem) == "usb" &&
                            parent_devtype && std::string(parent_devtype) == "usb_device") {
                            usb_device = parent_dev;
                            break;
                        }
                        parent_dev = udev_device_get_parent(parent_dev);
                    }
                }

                if (usb_device)
                {
                    core::connection::SerialPortInfo info;
                    info.systemLocation = devnode_str;
                    
                    const char* vid_str = get_udev_property(usb_device, "idVendor");
                    const char* pid_str = get_udev_property(usb_device, "idProduct");
                    const char* serial_str = get_udev_property(usb_device, "serial");

                    info.vendorIdentifier = hexToUint16(vid_str);
                    info.productIdentifier = hexToUint16(pid_str);
                    if (serial_str)
                    {
                        info.serialNumber = serial_str;
                    }
                    else // if no serial number is available from udev, try using the device node name
                    {
                        size_t lastSlash = devnode_str.rfind('/');
                        if (lastSlash != std::string::npos) {
                            info.serialNumber = devnode_str.substr(lastSlash + 1);
                        } else {
                            info.serialNumber = devnode_str;
                        }
                    }

                    ports.push_back(info);
                }
            }
        }
    }

    return ports;
}

} // namespace core::connection