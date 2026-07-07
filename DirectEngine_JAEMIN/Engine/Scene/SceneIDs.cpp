#include "SceneIDs.h"

#include <atomic>

namespace Engine::Scene
{
    namespace
    {
        std::atomic<ObjectID> g_nextObjectID{ 1 };
        std::atomic<ComponentID> g_nextComponentID{ 1 };

        template <typename ID>
        void ReserveID(std::atomic<ID>& nextID, ID id)
        {
            if (id == 0)
            {
                return;
            }

            ID current = nextID.load();
            while (current <= id && !nextID.compare_exchange_weak(current, id + 1))
            {
            }
        }
    }

    ObjectID GenerateObjectID()
    {
        return g_nextObjectID.fetch_add(1);
    }

    ComponentID GenerateComponentID()
    {
        return g_nextComponentID.fetch_add(1);
    }

    void ReserveObjectID(ObjectID id)
    {
        ReserveID(g_nextObjectID, id);
    }

    void ReserveComponentID(ComponentID id)
    {
        ReserveID(g_nextComponentID, id);
    }
}
