#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceBoolean.h"
#include "RiveViewModelInstanceColor.h"
#include "RiveViewModelInstanceEnum.h"
#include "RiveViewModelInstanceNumber.h"
#include "RiveViewModelInstanceString.h"
#include "RiveViewModelInstanceTrigger.h"
#include "RiveViewModelInstance.h"

namespace rive
{
class ViewModelInstanceBooleanRuntime;
class ViewModelInstanceColorRuntime;
class ViewModelInstanceEnumRuntime;
class ViewModelInstanceNumberRuntime;
class ViewModelInstanceStringRuntime;
class ViewModelInstanceTriggerRuntime;
class ViewModelInstanceRuntime;
} // namespace rive

// Template struct to resolve property types
template <typename T> struct PropertyTypeResolver
{
    using Type = void;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceBoolean>
{
    using Type = rive::ViewModelInstanceBooleanRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceColor>
{
    using Type = rive::ViewModelInstanceColorRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceEnum>
{
    using Type = rive::ViewModelInstanceEnumRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceNumber>
{
    using Type = rive::ViewModelInstanceNumberRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceString>
{
    using Type = rive::ViewModelInstanceStringRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstanceTrigger>
{
    using Type = rive::ViewModelInstanceTriggerRuntime*;
};

template <> struct PropertyTypeResolver<URiveViewModelInstance>
{
    using Type = rive::ViewModelInstanceRuntime*;
};
