# Property Source Map

This page is intentionally short.

## Property IDs

All WTC640 property identifiers are defined in:
- [propertyidwtc640.h](../../wtc640/include/core/wtc640/propertyidwtc640.h)
- [propertyidwtc640.cpp](../../wtc640/source/propertyidwtc640.cpp)

## Property Implementation

Adapter creation, read/write behavior, constraints, trigger interactions, and dynamic property wiring are implemented in:
- [propertieswtc640.cpp](../../wtc640/source/propertieswtc640.cpp)

If you need to understand why a property behaves a certain way, start in `propertieswtc640.cpp` and search for the property ID string from `propertyidwtc640`.
