#pragma once
#include "math/vec2.hpp"
#include "graphics/renderer.hpp"
#include "graphics/camera.hpp"
#include <vector>
#include <memory>
#include <string>

namespace loom {

class Node : public std::enable_shared_from_this<Node> {
public:
    std::string name;
    bool        visible = true;

    Node() = default;
    explicit Node(std::string name);
    virtual ~Node() = default;

    // Local transform
    Vec2  position() const { return m_position; }
    float rotation() const { return m_rotation; } // radians
    Vec2  scale()    const { return m_scale;    }
    float x()        const { return m_position.x; }
    float y()        const { return m_position.y; }

    void set_position(Vec2 pos)   { m_position = pos;    }
    void set_position(float x, float y) { m_position = {x, y}; }
    void set_rotation(float r)    { m_rotation = r;      }
    void set_scale(Vec2 s)        { m_scale = s;         }
    void set_scale(float s)       { m_scale = {s, s};    }
    void set_x(float x)           { m_position.x = x;   }
    void set_y(float y)           { m_position.y = y;   }

    // World-space transform (accumulated through parent chain)
    Vec2  world_position() const;
    float world_rotation() const;
    Vec2  world_scale()    const;

    // Scene graph
    void add_child(std::shared_ptr<Node> child);
    void remove_child(Node* child);
    void remove_from_parent();

    Node*                                     parent()   const { return m_parent; }
    const std::vector<std::shared_ptr<Node>>& children() const { return m_children; }

    // Override in subclasses
    virtual void update(float dt);
    virtual void draw(Renderer& renderer, const Camera& camera);

protected:
    Vec2  m_position = {0.f, 0.f};
    float m_rotation = 0.f;
    Vec2  m_scale    = {1.f, 1.f};

    Node*                            m_parent = nullptr;
    std::vector<std::shared_ptr<Node>> m_children;
};

} // namespace loom
