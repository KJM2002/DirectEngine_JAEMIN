#pragma once

namespace Game
{
    // Game-side entry point kept deliberately light while the engine foundation is built.
    class GameApp
    {
    public:
        void Update(float deltaTime);
    };
}
