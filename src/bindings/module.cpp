#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <algorithm>
#include <string>

#include "platform/window.hpp"
#include "graphics/renderer.hpp"
#include "graphics/texture.hpp"
#include "graphics/camera.hpp"
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include "scene/node.hpp"
#include "scene/sprite_node.hpp"
#include "scene/scene.hpp"
#include "scene/animation.hpp"
#include "scene/tilemap.hpp"
#include "text/font.hpp"
#include "text/text_node.hpp"
#include "input/input.hpp"
#include "physics/physics.hpp"
#include "audio/audio.hpp"
#include "assets/asset_manager.hpp"

namespace py = pybind11;

// ── Game base class (with all subsystems) ────────────────────────────────────

class Game {
public:
    virtual ~Game() = default;

    loom::Color        clear_color = loom::Color::cornflower();
    loom::Scene        scene;
    loom::PhysicsWorld physics;
    loom::AudioEngine  audio;
    loom::AssetManager assets;

    // Step physics & scene automatically each frame; override for custom logic
    bool auto_physics = true;
    bool auto_scene   = true;

    // Set to false from on_update() to exit the game loop programmatically.
    bool running = true;
    // Number of draw calls emitted in the previous frame (diagnostics).
    int  last_draw_calls = 0;

    virtual void on_start()          {}
    virtual void on_update(float dt) { (void)dt; }
    virtual void on_draw()           {}
    virtual void on_stop()           {}
};

class PyGame : public Game {
public:
    using Game::Game;
    void on_start()          override { PYBIND11_OVERRIDE(void, Game, on_start);     }
    void on_update(float dt) override { PYBIND11_OVERRIDE(void, Game, on_update, dt);}
    void on_draw()           override { PYBIND11_OVERRIDE(void, Game, on_draw);      }
    void on_stop()           override { PYBIND11_OVERRIDE(void, Game, on_stop);      }
};

// Trampoline for Python Node subclasses
class PyNode : public loom::Node {
public:
    using loom::Node::Node;
    void update(float dt) override {
        PYBIND11_OVERRIDE(void, loom::Node, update, dt);
    }
    void draw(loom::Renderer& r, const loom::Camera& c) override {
        PYBIND11_OVERRIDE(void, loom::Node, draw, r, c);
    }
};

// ── run() ─────────────────────────────────────────────────────────────────────

void run_game(Game& game, const std::string& title, int width, int height) {
    loom::Window   window(title, width, height);
    loom::Renderer renderer(window); // sets up sokol_gfx

    game.scene.camera.set_viewport(width, height);
    // Default: world (0,0) = screen top-left, matching pixel/screen coordinates
    game.scene.camera.set_position(loom::Vec2(width * 0.5f, height * 0.5f));

    game.on_start();

    Uint64 last_ticks = SDL_GetTicks();

    while (game.running && window.poll_events()) {
        loom::Input::update();

        Uint64 now = SDL_GetTicks();
        float  dt  = static_cast<float>(now - last_ticks) / 1000.f;
        last_ticks = now;
        dt = std::min(dt, 0.1f);

        game.on_update(dt);

        if (game.auto_physics) game.physics.step(dt);
        if (game.auto_scene)   game.scene.update(dt);

        renderer.begin_frame(game.clear_color);
        if (game.auto_scene) game.scene.draw(renderer);
        game.on_draw();
        renderer.end_frame();

        game.last_draw_calls = renderer.batcher().draw_calls();
    }

    game.on_stop();
}

// ── Module ────────────────────────────────────────────────────────────────────

PYBIND11_MODULE(loom2d_native, m) {
    m.doc() = "loom2d native engine module";

    // ── Vec2 ──────────────────────────────────────────────────────────────────
    py::class_<loom::Vec2>(m, "Vec2")
        .def(py::init<float,float>(), py::arg("x")=0.f, py::arg("y")=0.f)
        .def_readwrite("x", &loom::Vec2::x)
        .def_readwrite("y", &loom::Vec2::y)
        .def("__add__",  [](const loom::Vec2& a, const loom::Vec2& b){ return a + b; })
        .def("__sub__",  [](const loom::Vec2& a, const loom::Vec2& b){ return a - b; })
        .def("__mul__",  [](const loom::Vec2& a, float s){ return a * s; })
        .def("__rmul__", [](const loom::Vec2& a, float s){ return a * s; })
        .def("__truediv__",[](const loom::Vec2& a, float s){ return a / s; })
        .def("__neg__",  [](const loom::Vec2& a){ return -a; })
        .def("__eq__",   &loom::Vec2::operator==)
        .def("length",     &loom::Vec2::length)
        .def("length_sq",  &loom::Vec2::length_sq)
        .def("normalized", &loom::Vec2::normalized)
        .def("dot",        &loom::Vec2::dot)
        .def("distance",   &loom::Vec2::distance)
        .def("lerp",       &loom::Vec2::lerp)
        .def("rotated",    &loom::Vec2::rotated)
        .def_static("zero",  &loom::Vec2::zero)
        .def_static("one",   &loom::Vec2::one)
        .def_static("up",    &loom::Vec2::up)
        .def_static("down",  &loom::Vec2::down)
        .def_static("left",  &loom::Vec2::left)
        .def_static("right", &loom::Vec2::right)
        .def("__repr__", &loom::Vec2::to_string);

    // ── Rect ──────────────────────────────────────────────────────────────────
    py::class_<loom::Rect>(m, "Rect")
        .def(py::init<float,float,float,float>(),
             py::arg("x")=0.f, py::arg("y")=0.f,
             py::arg("w")=0.f, py::arg("h")=0.f)
        .def_readwrite("x", &loom::Rect::x)
        .def_readwrite("y", &loom::Rect::y)
        .def_readwrite("w", &loom::Rect::w)
        .def_readwrite("h", &loom::Rect::h)
        .def("left",        &loom::Rect::left)
        .def("right",       &loom::Rect::right)
        .def("top",         &loom::Rect::top)
        .def("bottom",      &loom::Rect::bottom)
        .def("center",      &loom::Rect::center)
        .def("contains",    py::overload_cast<const loom::Vec2&>(&loom::Rect::contains, py::const_))
        .def("intersects",  &loom::Rect::intersects)
        .def("intersection",&loom::Rect::intersection)
        .def("expanded",    &loom::Rect::expanded);

    // ── Color ─────────────────────────────────────────────────────────────────
    py::class_<loom::Color>(m, "Color")
        .def(py::init<float,float,float,float>(),
             py::arg("r")=0.f, py::arg("g")=0.f,
             py::arg("b")=0.f, py::arg("a")=1.f)
        .def_readwrite("r", &loom::Color::r)
        .def_readwrite("g", &loom::Color::g)
        .def_readwrite("b", &loom::Color::b)
        .def_readwrite("a", &loom::Color::a)
        .def_static("black",       &loom::Color::black)
        .def_static("white",       &loom::Color::white)
        .def_static("red",         &loom::Color::red)
        .def_static("green",       &loom::Color::green)
        .def_static("blue",        &loom::Color::blue)
        .def_static("yellow",      &loom::Color::yellow)
        .def_static("transparent", &loom::Color::transparent)
        .def_static("cornflower",  &loom::Color::cornflower)
        .def("__repr__", [](const loom::Color& c){
            return "Color(" + std::to_string(c.r) + ", " + std::to_string(c.g)
                 + ", " + std::to_string(c.b) + ", " + std::to_string(c.a) + ")";
        });

    // ── AnimationFrame ────────────────────────────────────────────────────────
    py::class_<loom::AnimationFrame>(m, "AnimationFrame")
        .def(py::init<>())
        .def_readwrite("source",   &loom::AnimationFrame::source)
        .def_readwrite("duration", &loom::AnimationFrame::duration);

    // ── Animation ─────────────────────────────────────────────────────────────
    py::class_<loom::Animation>(m, "Animation")
        .def(py::init<std::string, bool>(),
             py::arg("name"), py::arg("loop")=true)
        .def_readwrite("name", &loom::Animation::name)
        .def_readwrite("loop", &loom::Animation::loop)
        .def("add_frame",  &loom::Animation::add_frame,
             py::arg("source"), py::arg("duration")=0.1f)
        .def("add_strip",  &loom::Animation::add_strip,
             py::arg("sheet_w"), py::arg("sheet_h"),
             py::arg("frame_w"), py::arg("frame_h"),
             py::arg("cols"), py::arg("rows")=1,
             py::arg("frame_duration")=0.1f)
        .def("update",        &loom::Animation::update)
        .def("reset",         &loom::Animation::reset)
        .def("finished",      &loom::Animation::finished)
        .def("frame_index",   &loom::Animation::frame_index)
        .def("frame_count",   &loom::Animation::frame_count);

    // ── Camera ────────────────────────────────────────────────────────────────
    py::class_<loom::Camera>(m, "Camera")
        .def(py::init<>())
        .def_property("position",   &loom::Camera::position,
                                    &loom::Camera::set_position)
        .def_property("zoom",       &loom::Camera::zoom,
                                    &loom::Camera::set_zoom)
        .def_property("rotation",   &loom::Camera::rotation,
                                    &loom::Camera::set_rotation)
        .def("move",                &loom::Camera::move)
        .def("world_to_screen",     &loom::Camera::world_to_screen)
        .def("screen_to_world",     &loom::Camera::screen_to_world)
        .def("visible_rect",        &loom::Camera::visible_rect);

    // ── Texture ───────────────────────────────────────────────────────────────
    py::class_<loom::Texture, std::shared_ptr<loom::Texture>>(m, "Texture")
        .def_property_readonly("width",  &loom::Texture::width)
        .def_property_readonly("height", &loom::Texture::height)
        .def_property_readonly("path",   &loom::Texture::path);

    // ── Node ──────────────────────────────────────────────────────────────────
    py::class_<loom::Node, PyNode, std::shared_ptr<loom::Node>>(m, "Node")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def_readwrite("name",    &loom::Node::name)
        .def_readwrite("visible", &loom::Node::visible)
        .def_property("position",
            &loom::Node::position,
            py::overload_cast<loom::Vec2>(&loom::Node::set_position))
        .def_property("x", &loom::Node::x, &loom::Node::set_x)
        .def_property("y", &loom::Node::y, &loom::Node::set_y)
        .def_property("rotation",
            &loom::Node::rotation, &loom::Node::set_rotation)
        .def_property("scale",
            &loom::Node::scale,
            py::overload_cast<loom::Vec2>(&loom::Node::set_scale))
        .def("world_position", &loom::Node::world_position)
        .def("world_rotation", &loom::Node::world_rotation)
        .def("world_scale",    &loom::Node::world_scale)
        .def("add_child",      &loom::Node::add_child)
        .def("remove_from_parent", &loom::Node::remove_from_parent)
        .def("update",         &loom::Node::update)
        .def("children", [](const loom::Node& n) {
            // Return a copy so Python list lifetime is independent of the node
            return n.children();
        });

    // ── SpriteNode ────────────────────────────────────────────────────────────
    py::class_<loom::SpriteNode, loom::Node,
               std::shared_ptr<loom::SpriteNode>>(m, "SpriteNode")
        .def(py::init<std::shared_ptr<loom::Texture>>())
        .def_readwrite("tint",   &loom::SpriteNode::tint)
        .def_readwrite("origin", &loom::SpriteNode::origin)
        .def_readwrite("flip_x", &loom::SpriteNode::flip_x)
        .def_readwrite("flip_y", &loom::SpriteNode::flip_y)
        .def("set_texture", &loom::SpriteNode::set_texture)
        .def("set_source",  &loom::SpriteNode::set_source)
        .def("add_animation", &loom::SpriteNode::add_animation)
        .def("play",          &loom::SpriteNode::play)
        .def("stop",          &loom::SpriteNode::stop);

    // ── Tileset ───────────────────────────────────────────────────────────────
    py::class_<loom::Tileset, std::shared_ptr<loom::Tileset>>(m, "Tileset")
        .def(py::init<std::shared_ptr<loom::Texture>, int, int, int>(),
             py::arg("texture"), py::arg("tile_w"), py::arg("tile_h"),
             py::arg("first_gid")=1)
        .def_readwrite("texture",   &loom::Tileset::texture)
        .def_readwrite("tile_w",    &loom::Tileset::tile_w)
        .def_readwrite("tile_h",    &loom::Tileset::tile_h)
        .def_readwrite("first_gid", &loom::Tileset::first_gid)
        .def_readwrite("columns",   &loom::Tileset::columns)
        .def_readwrite("margin",    &loom::Tileset::margin)
        .def_readwrite("spacing",   &loom::Tileset::spacing)
        .def_property_readonly("tile_count", &loom::Tileset::tile_count)
        .def_property_readonly("last_gid",   &loom::Tileset::last_gid)
        .def("contains",   &loom::Tileset::contains)
        .def("source_for", &loom::Tileset::source_for);

    // ── TileLayer ─────────────────────────────────────────────────────────────
    py::class_<loom::TileLayer, std::shared_ptr<loom::TileLayer>>(m, "TileLayer")
        .def(py::init<std::string, int, int>(),
             py::arg("name"), py::arg("width"), py::arg("height"))
        .def_readwrite("name",    &loom::TileLayer::name)
        .def_readwrite("visible", &loom::TileLayer::visible)
        .def_readwrite("opacity", &loom::TileLayer::opacity)
        .def_property_readonly("width",  [](const loom::TileLayer& l){ return l.width;  })
        .def_property_readonly("height", [](const loom::TileLayer& l){ return l.height; })
        .def("at",   &loom::TileLayer::at)
        .def("set",  &loom::TileLayer::set)
        .def("fill", &loom::TileLayer::fill);

    // ── Tilemap ───────────────────────────────────────────────────────────────
    py::class_<loom::Tilemap, loom::Node,
               std::shared_ptr<loom::Tilemap>>(m, "Tilemap")
        .def(py::init<int, int, int, int>(),
             py::arg("tile_w"), py::arg("tile_h"),
             py::arg("width"), py::arg("height"))
        .def_readwrite("tile_w", &loom::Tilemap::tile_w)
        .def_readwrite("tile_h", &loom::Tilemap::tile_h)
        .def_property_readonly("width",  [](const loom::Tilemap& t){ return t.width;  })
        .def_property_readonly("height", [](const loom::Tilemap& t){ return t.height; })
        .def("add_tileset",
             py::overload_cast<std::shared_ptr<loom::Texture>, int, int, int>(
                 &loom::Tilemap::add_tileset),
             py::arg("texture"), py::arg("tile_w"), py::arg("tile_h"),
             py::arg("first_gid")=1)
        .def("tileset_for_gid", &loom::Tilemap::tileset_for_gid,
             py::return_value_policy::reference_internal)
        .def("add_layer",
             py::overload_cast<const std::string&>(&loom::Tilemap::add_layer),
             py::arg("name"))
        .def("layers",        &loom::Tilemap::layers)
        .def("layer",         &loom::Tilemap::layer, py::arg("index"))
        .def("layer_by_name", &loom::Tilemap::layer_by_name, py::arg("name"))
        .def("tile_to_world", &loom::Tilemap::tile_to_world,
             py::arg("tx"), py::arg("ty"))
        .def("world_to_tile", &loom::Tilemap::world_to_tile, py::arg("world"))
        .def("set_solid",     &loom::Tilemap::set_solid,
             py::arg("x"), py::arg("y"), py::arg("solid"))
        .def("is_solid",      &loom::Tilemap::is_solid, py::arg("x"), py::arg("y"))
        .def("set_collision_from_layer", &loom::Tilemap::set_collision_from_layer,
             py::arg("layer_index"))
        .def("clear_collision",   &loom::Tilemap::clear_collision)
        .def("rect_overlaps_solid", &loom::Tilemap::rect_overlaps_solid,
             py::arg("world_rect"))
        .def_property_readonly("tiles_drawn", &loom::Tilemap::tiles_drawn)
        .def_static("load", &loom::Tilemap::load, py::arg("path"));

    // ── TextAlign ─────────────────────────────────────────────────────────────
    py::enum_<loom::TextAlign>(m, "TextAlign")
        .value("Left",   loom::TextAlign::Left)
        .value("Center", loom::TextAlign::Center)
        .value("Right",  loom::TextAlign::Right)
        .export_values();

    // ── Font ──────────────────────────────────────────────────────────────────
    py::class_<loom::Font, std::shared_ptr<loom::Font>>(m, "Font")
        .def_static("load", &loom::Font::load,
                    py::arg("path"), py::arg("pixel_height"),
                    "Load a TTF/OTF and bake an ASCII atlas at the given pixel "
                    "height. Requires a running Game/Renderer.")
        .def_property_readonly("pixel_height", &loom::Font::pixel_height)
        .def_property_readonly("line_height",  &loom::Font::line_height)
        .def_property_readonly("ascent",       &loom::Font::ascent)
        .def("measure", &loom::Font::measure,
             py::arg("text"), py::arg("max_width") = 0.f,
             "Block (width, height) in pixels for the given text.")
        .def_static("wrap_lines", &loom::Font::wrap_lines,
                    py::arg("text"), py::arg("advances"), py::arg("max_width"),
                    "Greedy word-wrap helper: [start,end) byte ranges per line.");

    // ── TextNode ──────────────────────────────────────────────────────────────
    py::class_<loom::TextNode, loom::Node,
               std::shared_ptr<loom::TextNode>>(m, "TextNode")
        .def(py::init<std::shared_ptr<loom::Font>, std::string>(),
             py::arg("font"), py::arg("text") = "")
        .def_readwrite("color",  &loom::TextNode::color)
        .def_readwrite("origin", &loom::TextNode::origin)
        .def_property("text", &loom::TextNode::text, &loom::TextNode::set_text)
        .def_property("align", &loom::TextNode::align, &loom::TextNode::set_align)
        .def_property("max_width",
                      &loom::TextNode::max_width, &loom::TextNode::set_max_width)
        .def("set_font", &loom::TextNode::set_font)
        .def_property_readonly("font", &loom::TextNode::font)
        .def_property_readonly("size", &loom::TextNode::size);

    // ── Scene ─────────────────────────────────────────────────────────────────
    py::class_<loom::Scene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("camera", &loom::Scene::camera)
        .def("add",    &loom::Scene::add)
        .def("remove", &loom::Scene::remove)
        .def("clear",  &loom::Scene::clear)
        .def("update", &loom::Scene::update)
        .def("root",   &loom::Scene::root_ptr,
             py::return_value_policy::reference_internal);

    // ── Key enum ──────────────────────────────────────────────────────────────
    py::enum_<loom::Key>(m, "Key")
        .value("A", loom::Key::A) .value("B", loom::Key::B)
        .value("C", loom::Key::C) .value("D", loom::Key::D)
        .value("E", loom::Key::E) .value("F", loom::Key::F)
        .value("G", loom::Key::G) .value("H", loom::Key::H)
        .value("I", loom::Key::I) .value("J", loom::Key::J)
        .value("K", loom::Key::K) .value("L", loom::Key::L)
        .value("M", loom::Key::M) .value("N", loom::Key::N)
        .value("O", loom::Key::O) .value("P", loom::Key::P)
        .value("Q", loom::Key::Q) .value("R", loom::Key::R)
        .value("S", loom::Key::S) .value("T", loom::Key::T)
        .value("U", loom::Key::U) .value("V", loom::Key::V)
        .value("W", loom::Key::W) .value("X", loom::Key::X)
        .value("Y", loom::Key::Y) .value("Z", loom::Key::Z)
        .value("Up",     loom::Key::Up)    .value("Down",  loom::Key::Down)
        .value("Left",   loom::Key::Left)  .value("Right", loom::Key::Right)
        .value("Space",  loom::Key::Space) .value("Enter", loom::Key::Enter)
        .value("Escape", loom::Key::Escape).value("Tab",   loom::Key::Tab)
        .value("Shift",  loom::Key::Shift) .value("Ctrl",  loom::Key::Ctrl)
        .value("F1",     loom::Key::F1)    .value("F5",    loom::Key::F5)
        .value("F12",    loom::Key::F12)
        .export_values();

    py::enum_<loom::MouseButton>(m, "MouseButton")
        .value("Left",   loom::MouseButton::Left)
        .value("Middle", loom::MouseButton::Middle)
        .value("Right",  loom::MouseButton::Right)
        .export_values();

    // ── Input (static class) ─────────────────────────────────────────────────
    py::class_<loom::Input>(m, "Input")
        .def_static("key_down",     &loom::Input::key_down)
        .def_static("key_pressed",  &loom::Input::key_pressed)
        .def_static("key_released", &loom::Input::key_released)
        .def_static("mouse_position", &loom::Input::mouse_position)
        .def_static("mouse_down",     &loom::Input::mouse_down)
        .def_static("mouse_pressed",  &loom::Input::mouse_pressed)
        .def_static("inject_key_down", &loom::Input::inject_key_down)
        .def_static("inject_key_up",   &loom::Input::inject_key_up);

    // ── BodyType ──────────────────────────────────────────────────────────────
    py::enum_<loom::BodyType>(m, "BodyType")
        .value("Static",    loom::BodyType::Static)
        .value("Kinematic", loom::BodyType::Kinematic)
        .value("Dynamic",   loom::BodyType::Dynamic)
        .export_values();

    // ── Physics events ──────────────────────────────────────────────────────────
    py::class_<loom::ContactPair>(m, "ContactPair")
        .def_readonly("body_a", &loom::ContactPair::body_a,
                      py::return_value_policy::reference)
        .def_readonly("body_b", &loom::ContactPair::body_b,
                      py::return_value_policy::reference);

    py::class_<loom::SensorPair>(m, "SensorPair")
        .def_readonly("sensor",  &loom::SensorPair::sensor,
                      py::return_value_policy::reference)
        .def_readonly("visitor", &loom::SensorPair::visitor,
                      py::return_value_policy::reference);

    py::class_<loom::RaycastHit>(m, "RaycastHit")
        .def_readonly("hit",      &loom::RaycastHit::hit)
        .def_readonly("body",     &loom::RaycastHit::body,
                      py::return_value_policy::reference)
        .def_readonly("point",    &loom::RaycastHit::point)
        .def_readonly("normal",   &loom::RaycastHit::normal)
        .def_readonly("fraction", &loom::RaycastHit::fraction)
        .def("__bool__", [](const loom::RaycastHit& h){ return h.hit; });

    // ── PhysicsBody ───────────────────────────────────────────────────────────
    py::class_<loom::PhysicsBody>(m, "PhysicsBody")
        .def("add_box",    &loom::PhysicsBody::add_box,
             py::arg("half_w"), py::arg("half_h"),
             py::arg("density")=1.f, py::arg("friction")=0.3f,
             py::arg("restitution")=0.f, py::arg("is_sensor")=false)
        .def("add_circle", &loom::PhysicsBody::add_circle,
             py::arg("radius"),
             py::arg("density")=1.f, py::arg("friction")=0.3f,
             py::arg("restitution")=0.f, py::arg("is_sensor")=false)
        .def_property("tag", &loom::PhysicsBody::tag, &loom::PhysicsBody::set_tag)
        .def_property_readonly("position",        &loom::PhysicsBody::position)
        .def_property_readonly("rotation",        &loom::PhysicsBody::rotation)
        .def_property_readonly("linear_velocity", &loom::PhysicsBody::linear_velocity)
        .def("set_position",        &loom::PhysicsBody::set_position)
        .def("set_linear_velocity", &loom::PhysicsBody::set_linear_velocity)
        .def("apply_impulse",       &loom::PhysicsBody::apply_impulse)
        .def("apply_force",         &loom::PhysicsBody::apply_force);

    // ── PhysicsWorld ──────────────────────────────────────────────────────────
    py::class_<loom::PhysicsWorld>(m, "PhysicsWorld")
        .def(py::init<float, float>(),
             py::arg("gravity_x")=0.f, py::arg("gravity_y")=980.f)
        .def("step",         &loom::PhysicsWorld::step,
             py::arg("dt"), py::arg("sub_steps")=4)
        .def("create_body",  &loom::PhysicsWorld::create_body,
             py::arg("type"), py::arg("position"),
             py::return_value_policy::reference)
        .def("destroy_body", &loom::PhysicsWorld::destroy_body)
        .def("raycast",      &loom::PhysicsWorld::raycast,
             py::arg("p1"), py::arg("p2"))
        .def_property_readonly("contact_begins", &loom::PhysicsWorld::contact_begins)
        .def_property_readonly("contact_ends",   &loom::PhysicsWorld::contact_ends)
        .def_property_readonly("sensor_begins",  &loom::PhysicsWorld::sensor_begins)
        .def_property_readonly("sensor_ends",    &loom::PhysicsWorld::sensor_ends)
        .def_readwrite("on_contact_begin", &loom::PhysicsWorld::on_contact_begin)
        .def_readwrite("on_contact_end",   &loom::PhysicsWorld::on_contact_end)
        .def_readwrite("on_sensor_begin",  &loom::PhysicsWorld::on_sensor_begin)
        .def_readwrite("on_sensor_end",    &loom::PhysicsWorld::on_sensor_end);

    // ── AudioEngine ───────────────────────────────────────────────────────────
    py::class_<loom::AudioEngine>(m, "AudioEngine")
        .def(py::init<>())
        .def_property_readonly("initialized", &loom::AudioEngine::initialized)
        .def("play_sound",        &loom::AudioEngine::play_sound,
             py::arg("path"), py::arg("volume")=1.f)
        .def("play_music",        &loom::AudioEngine::play_music,
             py::arg("path"), py::arg("volume")=1.f, py::arg("loop")=true)
        .def("stop_music",        &loom::AudioEngine::stop_music)
        .def("set_music_volume",  &loom::AudioEngine::set_music_volume)
        .def("music_playing",     &loom::AudioEngine::music_playing)
        .def("set_master_volume", &loom::AudioEngine::set_master_volume);

    // ── AssetManager ──────────────────────────────────────────────────────────
    py::class_<loom::AssetManager>(m, "AssetManager")
        .def(py::init<>())
        .def("texture", &loom::AssetManager::texture)
        .def("clear",   &loom::AssetManager::clear);

    // ── Game ──────────────────────────────────────────────────────────────────
    py::class_<Game, PyGame>(m, "Game")
        .def(py::init<>())
        .def_readwrite("clear_color",  &Game::clear_color)
        .def_readwrite("scene",        &Game::scene)
        .def_readwrite("auto_physics", &Game::auto_physics)
        .def_readwrite("auto_scene",   &Game::auto_scene)
        .def_readwrite("running",      &Game::running)
        .def_property_readonly("last_draw_calls", [](Game& g) { return g.last_draw_calls; })
        .def_property_readonly("physics", [](Game& g) -> loom::PhysicsWorld& { return g.physics; },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("audio",   [](Game& g) -> loom::AudioEngine&  { return g.audio;   },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("assets",  [](Game& g) -> loom::AssetManager& { return g.assets;  },
                               py::return_value_policy::reference_internal)
        .def("on_start",  &Game::on_start)
        .def("on_update", &Game::on_update, py::arg("dt"))
        .def("on_draw",   &Game::on_draw)
        .def("on_stop",   &Game::on_stop);

    // ── run() ─────────────────────────────────────────────────────────────────
    m.def("run", &run_game,
          py::arg("game"),
          py::arg("title")  = "loom2d",
          py::arg("width")  = 800,
          py::arg("height") = 600,
          "Start the game loop. Blocks until the window closes.");
}
