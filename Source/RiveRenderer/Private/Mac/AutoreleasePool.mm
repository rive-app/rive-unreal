// Copyright Rive, Inc. All rights reserved.

#include "AutoreleasePool.h"
#if PLATFORM_APPLE
AutoreleasePool::AutoreleasePool() : Pool([NSAutoreleasePool new]) {}
AutoreleasePool::~AutoreleasePool() { [this->Pool drain]; }
#else
AutoreleasePool::AutoreleasePool() {}
AutoreleasePool::~AutoreleasePool() {}
#endif
