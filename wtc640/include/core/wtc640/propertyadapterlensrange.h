#ifndef CORE_PROPERTYADAPTERLENSRANGE_H
#define CORE_PROPERTYADAPTERLENSRANGE_H

#include "core/wtc640/propertieswtc640.h"
#include "core/properties/propertyadaptervaluederived.h"


namespace core
{

class PropertyAdapterCurrentLensRange : public PropertyAdapterValueDerived<PropertiesWtc640::PresetId>
{
    using ValueType = PropertiesWtc640::PresetId;
    using BaseClass = PropertyAdapterValueDerived<ValueType>;

public:
    using PresetIndexAdapterPtr = std::shared_ptr<PropertyAdapterValue<unsigned>>;
    explicit PropertyAdapterCurrentLensRange(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                             const Properties::AdapterTaskCreator& taskCreator,
                                             const std::shared_ptr<PropertyValues>& propertyValues,
                                             const PresetIndexAdapterPtr& presetIndexAdapter);

    using PresetIdAdapterPtr = std::shared_ptr<PropertyAdapterValue<PropertiesWtc640::PresetId>>;
    void setPresetIdAdapters(const std::vector<PresetIdAdapterPtr>& presetIdAdapters);

protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const override;

    std::optional<unsigned> getPresetIndex(const ValueType& value, const PropertyValues::Transaction& transaction) const;

private:
    PropertyId m_presetIndexPropertyId;
    std::vector<PresetIdAdapterPtr> m_presetIdAdapters;
};


class PropertyAdapterSelectedLensRange : public PropertyAdapterCurrentLensRange
{
    using ValueType = PropertiesWtc640::PresetId;
    using BaseClass = PropertyAdapterCurrentLensRange;

public:
    explicit PropertyAdapterSelectedLensRange(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                              const Properties::AdapterTaskCreator& taskCreator,
                                              const std::shared_ptr<PropertyValues>& propertyValues,
                                              const PresetIndexAdapterPtr& presetIndexAdapter);

    virtual VoidResult setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction) override;
    virtual RankedValidationResult validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const override;
    virtual VoidResult getLastWriteResult() const override;

private:
    PresetIndexAdapterPtr m_presetIndexAdapter;
};


class PropertyAdapterAllValidLensRanges : public PropertyAdapterValueDerived<std::vector<PropertiesWtc640::PresetId>>
{
    using ValueType = std::vector<PropertiesWtc640::PresetId>;
    using BaseClass = PropertyAdapterValueDerived<ValueType>;

public:
    explicit PropertyAdapterAllValidLensRanges(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                               const Properties::AdapterTaskCreator& taskCreator,
                                               const std::shared_ptr<PropertyValues>& propertyValues);

    using PresetIdAdapterPtr = std::shared_ptr<PropertyAdapterValue<PropertiesWtc640::PresetId>>;
    void setPresetIdAdapters(const std::vector<PresetIdAdapterPtr>& presetIdAdapters);

protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const override;

private:
    std::vector<PresetIdAdapterPtr> m_presetIdAdapters;
};

} // namespace core

#endif // CORE_PROPERTYADAPTERLENSRANGE_H
