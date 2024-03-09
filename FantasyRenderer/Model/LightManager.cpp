#include "LightManager.h"

LightManager::LightManager()
{
    AddLight(ELightType::Direct);
}


void LightManager::AddLight(ELightType InType)
{
    switch (InType)
    {
    case ELightType::Direct: DirectLights.emplace_back(); break;
    case ELightType::Point: PointLights.emplace_back(); break;
    case ELightType::Spot: SpotLights.emplace_back(); break;
    }
}
