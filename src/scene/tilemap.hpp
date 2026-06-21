#pragma once
#include "scene/node.hpp"
#include "graphics/texture.hpp"
#include "graphics/color.hpp"
#include "math/rect.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace loom {

// ── Pure TMX/TSX parse results (GPU-free, unit-testable) ─────────────────────
// These mirror the subset of the Tiled (.tmx/.tsx) format loom2d understands:
// orthogonal maps, CSV-encoded layer data, one or more tilesets (embedded or
// external .tsx). Loading the actual GPU textures is a separate step so the
// parser can be tested without a GL context.

struct TmxTileset {
    int         first_gid  = 1;
    std::string source;     // external .tsx path (empty when embedded)
    std::string image;      // tileset image path (resolved by the loader)
    int         tile_w     = 0, tile_h = 0;
    int         columns    = 0;
    int         tile_count = 0;
    int         margin     = 0, spacing = 0;
    int         image_w    = 0, image_h = 0;
};

struct TmxLayer {
    std::string      name;
    int              width   = 0, height = 0;
    bool             visible = true;
    float            opacity = 1.f;
    std::vector<int> gids;   // row-major, length width*height (0 = empty)
};

struct TmxMap {
    int                     width  = 0, height = 0; // in tiles
    int                     tile_w = 0, tile_h = 0;
    std::vector<TmxTileset> tilesets;
    std::vector<TmxLayer>   layers;
};

// Throws std::runtime_error on malformed input or unsupported encoding.
TmxMap     parse_tmx(const std::string& xml);
TmxTileset parse_tsx(const std::string& xml);

// ── Runtime tile data ────────────────────────────────────────────────────────

// A tileset maps global tile ids (gids) to sub-rectangles of one texture.
// gids are 1-based and offset by first_gid; gid 0 always means "empty".
class Tileset {
public:
    std::shared_ptr<Texture> texture;
    int tile_w    = 0, tile_h = 0;
    int first_gid = 1;
    int columns   = 0;            // tiles per row; 0 = derive from texture width
    int margin    = 0, spacing = 0;

    Tileset() = default;
    Tileset(std::shared_ptr<Texture> tex, int tile_w, int tile_h, int first_gid = 1);

    int  columns_resolved() const; // falls back to texture width when columns==0
    int  tile_count()       const; // total tiles in the image
    int  last_gid()         const { return first_gid + tile_count() - 1; }
    bool contains(int gid)  const;

    // Source sub-rectangle (pixels) of this tileset's texture for a gid.
    Rect source_for(int gid) const;
};

// A single grid of tile ids. Multiple layers stack to form a map
// (e.g. ground / objects / overhead).
class TileLayer {
public:
    std::string      name;
    int              width   = 0, height = 0;
    bool             visible = true;
    float            opacity = 1.f;
    std::vector<int> gids;

    TileLayer() = default;
    TileLayer(std::string name, int width, int height);

    bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    int  at(int x, int y) const;        // 0 if out of bounds
    void set(int x, int y, int gid);
    void fill(int gid);
};

// ── Tilemap node ─────────────────────────────────────────────────────────────
// A Node that batch-renders many tiles with viewport culling: only the tiles
// inside the camera's visible rect are submitted, so a huge map still costs ~1
// draw call per tileset. Also owns a grid collision mask (cheap solid lookups
// for tile worlds, independent of Box2D).
//
// Tile placement and collision honour the node's world position and (uniform)
// world scale; rotation is ignored for tiles (tilemaps are axis-aligned).
class Tilemap : public Node {
public:
    int tile_w = 0, tile_h = 0; // pixel size of a tile
    int width  = 0, height = 0; // map size in tiles

    Tilemap() = default;
    Tilemap(int tile_w, int tile_h, int width, int height);

    // Tilesets
    std::shared_ptr<Tileset> add_tileset(std::shared_ptr<Texture> tex,
                                         int tile_w, int tile_h, int first_gid = 1);
    void                     add_tileset(std::shared_ptr<Tileset> ts);
    const std::vector<std::shared_ptr<Tileset>>& tilesets() const { return m_tilesets; }
    Tileset*                 tileset_for_gid(int gid) const;

    // Layers
    std::shared_ptr<TileLayer> add_layer(const std::string& name);
    void                       add_layer(std::shared_ptr<TileLayer> layer);
    const std::vector<std::shared_ptr<TileLayer>>& layers() const { return m_layers; }
    std::shared_ptr<TileLayer> layer(int index) const;
    std::shared_ptr<TileLayer> layer_by_name(const std::string& name) const;

    // Coordinate conversion (honours node world position + scale)
    Vec2 tile_to_world(int tx, int ty) const; // top-left corner of the tile
    Vec2 world_to_tile(Vec2 world) const;      // floored tile coordinates

    // Grid collision
    void set_solid(int x, int y, bool solid);
    bool is_solid(int x, int y) const;
    void set_collision_from_layer(int layer_index); // any nonzero gid => solid
    void clear_collision();
    bool rect_overlaps_solid(const Rect& world_rect) const;

    int  tiles_drawn() const { return m_tiles_drawn; } // from the last draw()

    void draw(Renderer& renderer, const Camera& camera) override;

    // Load an orthogonal Tiled .tmx map (CSV encoding). Requires a Renderer
    // (uploads tileset textures via Texture::load).
    static std::shared_ptr<Tilemap> load(const std::string& path);

private:
    std::vector<std::shared_ptr<Tileset>>   m_tilesets;
    std::vector<std::shared_ptr<TileLayer>> m_layers;
    std::vector<std::uint8_t>               m_solid; // width*height, 1 = solid
    mutable int                             m_tiles_drawn = 0;
};

} // namespace loom
