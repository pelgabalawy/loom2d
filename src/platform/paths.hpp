#pragma once
#include <string>

namespace loom {

// The directory a game may write its save files to, created if it isn't there.
//
// Every OS puts this somewhere different (%APPDATA% on Windows,
// ~/Library/Application Support on macOS, $XDG_DATA_HOME on Linux, the app's
// sandbox on Android/iOS), and only the OS knows which — so we ask SDL rather
// than guess. Writing next to the executable, the obvious-looking alternative,
// fails the moment the game is installed somewhere read-only.
//
// Returns a path with a trailing separator, or "" if the OS refused.
std::string save_dir(const std::string& org, const std::string& app);

} // namespace loom
