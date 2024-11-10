// Copyright Rive, Inc. All rights reserved.

#pragma once
class AutoreleasePool
{
public:
#if PLATFORM_APPLE
    AutoreleasePool() : Pool([NSAutoreleasePool new]) {}
    ~AutoreleasePool() { [this->Pool drain]; }
#else
    AutoreleasePool() {}
    ~AutoreleasePool() {}
#endif

private:
#if PLATFORM_APPLE
    id Pool;
#endif
private:
    AutoreleasePool(const AutoreleasePool&);
    AutoreleasePool& operator=(const AutoreleasePool&);
};
