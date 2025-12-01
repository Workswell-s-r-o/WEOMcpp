#ifndef CORE_PROPERTYVALUEBASE_H
#define CORE_PROPERTYVALUEBASE_H

#include "core/properties/propertyid.h"
#include "core/misc/result.h"

#include <boost/signals2.hpp>


namespace core
{

class PropertyValueBase
{

public:
    explicit PropertyValueBase(PropertyId propertyId);

    PropertyId getPropertyId() const;

    virtual void resetValue() = 0;

    virtual bool hasValueResult() const = 0;
    virtual VoidResult getValidationResult() const = 0;
    virtual std::string getValueAsString() const = 0;

    virtual bool valueEquals(const PropertyValueBase* other) const = 0;

    boost::signals2::signal<void(size_t)> valueChanged;

private:
    PropertyId m_propertyId;
};

} // namespace core

#endif // CORE_PROPERTYVALUEBASE_H
