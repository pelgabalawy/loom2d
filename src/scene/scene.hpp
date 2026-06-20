#pragma once
#include "scene/node.hpp"
#include "graphics/camera.hpp"
#include <memory>

namespace loom {

class Scene {
public:
    Camera camera;

    Scene();

    void add(std::shared_ptr<Node> node);
    void remove(Node* node);
    void clear();

    void update(float dt);
    void draw(Renderer& renderer);

    // C++ convenience — returns a reference into the shared_ptr root
    Node& root() { return *m_root; }

    // Python-safe accessor — returns the shared_ptr itself
    std::shared_ptr<Node> root_ptr() { return m_root; }

private:
    std::shared_ptr<Node> m_root;
};

} // namespace loom
