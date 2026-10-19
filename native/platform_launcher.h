#pragma once
#include <functional>
#include <string>

using MeleeDiscValidator = std::function<std::string(const std::string&)>;
std::string MeleeLaunchDisc(MeleeDiscValidator validate, bool forceSetup = false);
#ifdef __APPLE__
void MeleeInstallAppMenu(std::function<void()> changeDisc);
#else
std::string MeleeChooseDisc(const std::string& error = {});
void MeleeShowLaunchError(const std::string& error);
#endif
void MeleePrepareAppLogging();

// Linux returns XDG paths and the web build MEMFS paths; other platforms
// retain Aurora defaults.
#if defined(__linux__) || defined(__EMSCRIPTEN__)
std::string MeleeConfigPath();
std::string MeleeCachePath();
#endif
