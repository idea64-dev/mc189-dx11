#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/CrashHandler.h"

// Installed before the other static initializers, so a crash there is reported with its stack too.
#pragma warning(disable : 4073)
#pragma init_seg(compiler)
static const bool crashHandlerInstalled = [] {
    mc::CrashHandler::installStackPrinter();
    return true;
}();
