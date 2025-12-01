#include "core/properties/propertyvaluebase.h"


namespace core
{

PropertyValueBase::PropertyValueBase(PropertyId propertyId) :
    m_propertyId(propertyId)
{
}

PropertyId PropertyValueBase::getPropertyId() const
{
    return m_propertyId;
}

} // namespace core
