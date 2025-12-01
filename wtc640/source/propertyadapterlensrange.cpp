#include "core/wtc640/propertyadapterlensrange.h"
#include "core/utils.h"


namespace core
{

PropertyAdapterCurrentLensRange::PropertyAdapterCurrentLensRange(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                 const Properties::AdapterTaskCreator& taskCreator,
                                                                 const std::shared_ptr<PropertyValues>& propertyValues,
                                                                 const PresetIndexAdapterPtr& presetIndexAdapter) :
    BaseClass(propertyId, getStatusForDeviceFunction,
              propertyValues,
              {presetIndexAdapter}),
    m_presetIndexPropertyId(presetIndexAdapter->getPropertyId())
{
}

void PropertyAdapterCurrentLensRange::setPresetIdAdapters(const std::vector<PresetIdAdapterPtr>& presetIdAdapters)
{
    for (const auto& sourceAdapter : m_presetIdAdapters)
    {
        removeSourceAdapter(sourceAdapter);
    }

    m_presetIdAdapters = presetIdAdapters;

    for (const auto& sourceAdapter : m_presetIdAdapters)
    {
        addSourceAdapter(sourceAdapter);
    }
}

OptionalResult<PropertyAdapterCurrentLensRange::ValueType> PropertyAdapterCurrentLensRange::getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                                                                         const PropertyValues::Transaction& transaction) const
{
    using ReturnType = OptionalResult<ValueType>;

    const auto currentPresetIndex = transaction.getValue<unsigned>(m_presetIndexPropertyId);
    if (currentPresetIndex.containsError())
    {
        return ReturnType::createFromError(currentPresetIndex.getResult());
    }
    else if (!currentPresetIndex.containsValue())
    {
        return std::nullopt;
    }

    if (currentPresetIndex.getValue() >= m_presetIdAdapters.size())
    {
        return ReturnType::createError("Invalid preset", utils::format("preset index: {} presets count: {}", currentPresetIndex.getValue(), m_presetIdAdapters.size()));
    }

    return transaction.getValue<PropertiesWtc640::PresetId>(m_presetIdAdapters.at(currentPresetIndex.getValue())->getPropertyId());
}

std::optional<unsigned int> PropertyAdapterCurrentLensRange::getPresetIndex(const ValueType& value, const PropertyValues::Transaction& transaction) const
{
    for (unsigned i = 0; i < m_presetIdAdapters.size(); ++i)
    {
        const auto presetId = m_presetIdAdapters.at(i)->getValue(transaction);
        if (presetId.containsValue() && presetId.getValue() == value)
        {
            return i;
        }
    }

    return std::nullopt;
}


PropertyAdapterSelectedLensRange::PropertyAdapterSelectedLensRange(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                   const Properties::AdapterTaskCreator& taskCreator,
                                                                   const std::shared_ptr<PropertyValues>& propertyValues,
                                                                   const PresetIndexAdapterPtr& presetIndexAdapter) :
    BaseClass(propertyId, getStatusForDeviceFunction,
              taskCreator,
              propertyValues,
              presetIndexAdapter),
    m_presetIndexAdapter(presetIndexAdapter)
{
    presetIndexAdapter.get()->valueWriteFinished.connect([this](size_t propertyInternalId, const std::string& generalErrorMessage, const std::string& detailErrorMessage)
    {
        valueWriteFinished(getPropertyId().getInternalId(), generalErrorMessage, detailErrorMessage);
    });
}

VoidResult PropertyAdapterSelectedLensRange::setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction)
{
    if (!isWritable(transaction))
    {
        return VoidResult::createError("Unable to write!", utils::format("adapter in non-writable mode - property: {}", getPropertyId().getIdString()));
    }

    const RankedValidationResult result = validateValueForWrite(newValue, transaction);
    if (!result.isAcceptable())
    {
        assert(!result.getResult().isOk());
        return result.getResult();
    }

    const OptionalResult<ValueType> oldValue = transaction.getValue<ValueType>(getPropertyId());
    if (oldValue.containsValue() && newValue == oldValue.getValue())
    {
        return VoidResult::createOk();
    }

    const auto presetIndex = getPresetIndex(newValue, transaction);
    assert(presetIndex.has_value());
    return m_presetIndexAdapter->setValue(presetIndex.value(), transaction);
}

RankedValidationResult PropertyAdapterSelectedLensRange::validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const
{
    if (value.lens == Lens::Item::NOT_DEFINED)
    {
        return RankedValidationResult::createError("Lens not defined!");
    }

    if (value.range == Range::Item::NOT_DEFINED)
    {
        return RankedValidationResult::createError("Range not defined!");
    }

    if (!getPresetIndex(value, transaction).has_value())
    {
        return RankedValidationResult::createError("Invalid lens range combination!", utils::format("preset for lens: {} and range: {} not found", Lens::ALL_ITEMS.at(value.lens).userName, Range::ALL_ITEMS.at(value.range).userName));
    }

    return BaseClass::validateValueForWrite(value, transaction);
}

VoidResult PropertyAdapterSelectedLensRange::getLastWriteResult() const
{
    return m_presetIndexAdapter->getLastWriteResult();
}

PropertyAdapterAllValidLensRanges::PropertyAdapterAllValidLensRanges(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                     const Properties::AdapterTaskCreator& taskCreator,
                                                                     const std::shared_ptr<PropertyValues>& propertyValues) :
    BaseClass(propertyId, getStatusForDeviceFunction,
              propertyValues,
              {})
{
}

void PropertyAdapterAllValidLensRanges::setPresetIdAdapters(const std::vector<PresetIdAdapterPtr>& presetIdAdapters)
{
    for (const auto& sourceAdapter : m_presetIdAdapters)
    {
        removeSourceAdapter(sourceAdapter);
    }

    m_presetIdAdapters = presetIdAdapters;

    for (const auto& sourceAdapter : m_presetIdAdapters)
    {
        addSourceAdapter(sourceAdapter);
    }
}

OptionalResult<PropertyAdapterAllValidLensRanges::ValueType> PropertyAdapterAllValidLensRanges::getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                                                                             const PropertyValues::Transaction& transaction) const
{
    using ReturnType = OptionalResult<ValueType>;

    ValueType validPresetIds;

    for (const auto& presetIdAdapter : m_presetIdAdapters)
    {
        const auto presetId = transaction.getValue<PropertiesWtc640::PresetId>(presetIdAdapter->getPropertyId());
        if (presetId.containsError())
        {
            return ReturnType::createFromError(presetId.getResult());
        }

        if (!presetId.containsValue())
        {
            return std::nullopt;
        }

        if (presetId.getValue().isDefined())
        {
            validPresetIds.push_back(presetId.getValue());
        }
    }

    return validPresetIds;
}

} // namespace core
