#ifndef CORE_VIDEOFORMATADAPTER_H
#define CORE_VIDEOFORMATADAPTER_H

#include "devicewtc640.h"

#include "core/properties/propertyadaptervaluedevicesimple.h"

namespace core {
class PropertiesWtc640;

class VideoFormatAdapter final : public PropertyAdapterValueDeviceSimple<VideoFormat::Item>
{
    using BaseClass = PropertyAdapterValueDeviceSimple<VideoFormat::Item>;
public:
    VideoFormatAdapter(PropertiesWtc640* properties,
                       PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                       const Properties::AdapterTaskCreator& taskCreator,
                       const connection::AddressRanges& addressRanges,
                       const ValueReader& valueReader, const ValueWriter& valueWriter,
                       const typename BaseClass::TransformFunction& transformFunction);
protected:
    void addWriteTask(const VideoFormat::Item& value, const PropertyValues::Transaction& transaction) override;

private:
    PropertiesWtc640* m_properties;
};
} // namespace core
#endif // VIDEOFORMATADAPTER_H
