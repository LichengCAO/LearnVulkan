#include "material.h"
#define COMPONENT_IMPLEMENTATION
#include "component.h"

COMPONENT_DEFINITION(Component, MaterialBaseColor); 
COMPONENT_DEFINITION(Component, MaterialMetallic);
COMPONENT_DEFINITION(Component, MaterialRoughness);
COMPONENT_DEFINITION(Component, MaterialNormal);
COMPONENT_DEFINITION(Component, MaterialEmissive);
COMPONENT_DEFINITION(Component, MaterialOcclusion);
