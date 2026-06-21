#include "scene/scene.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"

namespace loom {

Scene::Scene() : m_root(std::make_shared<Node>("root")) {}

void Scene::add(std::shared_ptr<Node> node) {
    m_root->add_child(std::move(node));
}

void Scene::remove(Node* node) {
    m_root->remove_child(node);
}

void Scene::clear() {
    while (!m_root->children().empty())
        m_root->remove_child(m_root->children().back().get());
}

void Scene::update(float dt) {
    m_root->update(dt);
}

void Scene::draw(Renderer& renderer) {
    // Push the camera transform to the GPU once per frame, then submit sprites.
    renderer.batcher().set_view_projection(camera.view_projection());
    m_root->draw(renderer, camera);
}

} // namespace loom
