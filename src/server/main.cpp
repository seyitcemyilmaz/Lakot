#include "Server.h"

int main()
{
    lakot::Server tServer;

    if (tServer.initialize())
    {
        tServer.run();
    }

    return 0;
}
