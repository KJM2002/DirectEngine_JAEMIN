#include "Engine/Core/Application.h"

int main()
{
    Engine::Core::Application application;
    if (!application.Initialize())
    {
        return 1;
    }

    const int result = application.Run();
    application.Shutdown();

    return result;
}
