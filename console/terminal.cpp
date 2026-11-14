#include "terminal.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace console {

void prepareTerminal()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

}
