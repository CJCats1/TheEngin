#pragma once
#include <TheEngine/Core/Export.h>

namespace TheEngine {
    class EntityManager;

    class FirmGuySystem {
    public:
        static void update(EntityManager& manager, float dt);
    };
}
