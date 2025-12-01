#include "core/wtc640/videoformatadapter.h"
#include "core/wtc640/propertyidwtc640.h"
#include "core/wtc640/propertieswtc640.h"

namespace core {

VideoFormatAdapter::VideoFormatAdapter(PropertiesWtc640* properties,
                                       PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                       const Properties::AdapterTaskCreator& taskCreator,
                                       const connection::AddressRanges& addressRanges,
                                       const ValueReader& valueReader, const ValueWriter& valueWriter,
                                       const typename BaseClass::TransformFunction& transformFunction)
    : BaseClass(propertyId, getStatusForDeviceFunction, taskCreator, addressRanges, valueReader, valueWriter, transformFunction)
    , m_properties(properties)
{}

void VideoFormatAdapter::addWriteTask(const VideoFormat::Item& value, const PropertyValues::Transaction& transaction)
{
    if (m_properties)
    {
        const auto pluginTypeId = PropertyIdWtc640::PLUGIN_TYPE;
        const auto pluginTypeResult = transaction.getValue<Plugin::Item>(pluginTypeId);
        if (pluginTypeResult.containsValue() && pluginTypeResult.getValue() == Plugin::Item::USB)
        {
            const auto task = [this, value](connection::IDeviceInterface* device, const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
            {
                auto stream = m_properties->getOrCreateStreamImpl();
                if (stream.isOk())
                {
                    static_cast<void>(stream.getValue()->stopStream());
                }

                auto writeResult = VoidResult::createOk();
                const auto transaction = getTransactionFunction();

                this->updateValueAfterWrite(writeResult, value, transaction.getValuesTransaction());

                return writeResult;
            };
            this->getTaskCreator().createTaskSimpleWrite(this->getAddressRanges(), task);
            return;
        }
    }
    BaseClass::addWriteTask(value, transaction);
}
};
