#ifdef _WIN32
#include <windows.h>

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  return TRUE;
}
#endif
