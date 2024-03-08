// Copyright Rive, Inc. All rights reserved.

#pragma once
class AutoreleasePool {
public:
    AutoreleasePool();
    ~AutoreleasePool();

private:
#if PLATFORM_APPLE
    id Pool;
#endif
private:
    AutoreleasePool(const AutoreleasePool&);
    AutoreleasePool& operator=(const AutoreleasePool&);
};
