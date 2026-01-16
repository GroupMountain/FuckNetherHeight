#pragma once
#include <memory>
class FuckNetherHeightHooks {
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    FuckNetherHeightHooks();
    ~FuckNetherHeightHooks();

public:
    static FuckNetherHeightHooks& getInstance();
};