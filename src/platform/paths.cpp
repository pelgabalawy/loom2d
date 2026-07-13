#include "platform/paths.hpp"
#include <SDL3/SDL.h>

namespace loom {

std::string save_dir(const std::string& org, const std::string& app) {
    char* path = SDL_GetPrefPath(org.c_str(), app.c_str());
    if (!path) return "";
    std::string result(path);
    SDL_free(path);
    return result;
}

} // namespace loom
