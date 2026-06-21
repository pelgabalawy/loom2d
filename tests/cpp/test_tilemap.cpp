#include <gtest/gtest.h>
#include "scene/tilemap.hpp"
#include "math/rect.hpp"

using namespace loom;

// ── Tileset source rects (GPU-free: columns set explicitly) ──────────────────

TEST(Tileset, SourceForFirstTile) {
    Tileset ts(nullptr, 32, 32, 1);
    ts.columns = 4;
    Rect r = ts.source_for(1); // first gid -> top-left tile
    EXPECT_FLOAT_EQ(r.x, 0.f);
    EXPECT_FLOAT_EQ(r.y, 0.f);
    EXPECT_FLOAT_EQ(r.w, 32.f);
    EXPECT_FLOAT_EQ(r.h, 32.f);
}

TEST(Tileset, SourceForWrapsToNextRow) {
    Tileset ts(nullptr, 32, 32, 1);
    ts.columns = 4;
    Rect r = ts.source_for(6); // local index 5 -> col 1, row 1
    EXPECT_FLOAT_EQ(r.x, 32.f);
    EXPECT_FLOAT_EQ(r.y, 32.f);
}

TEST(Tileset, SourceHonoursMarginAndSpacing) {
    Tileset ts(nullptr, 16, 16, 1);
    ts.columns = 8;
    ts.margin  = 2;
    ts.spacing = 1;
    Rect r = ts.source_for(2); // local index 1 -> col 1
    EXPECT_FLOAT_EQ(r.x, 2.f + 1 * (16 + 1)); // margin + col*(tile+spacing) = 19
    EXPECT_FLOAT_EQ(r.y, 2.f);
}

TEST(Tileset, FirstGidOffsetsLookup) {
    Tileset ts(nullptr, 32, 32, 5);
    ts.columns = 4;
    Rect r = ts.source_for(5); // first gid maps to tile 0
    EXPECT_FLOAT_EQ(r.x, 0.f);
    EXPECT_FLOAT_EQ(r.y, 0.f);
}

// ── TileLayer ────────────────────────────────────────────────────────────────

TEST(TileLayer, SetAndGet) {
    TileLayer l("ground", 4, 3);
    l.set(2, 1, 7);
    EXPECT_EQ(l.at(2, 1), 7);
    EXPECT_EQ(l.at(0, 0), 0);
}

TEST(TileLayer, OutOfBoundsReadsZero) {
    TileLayer l("ground", 4, 3);
    EXPECT_EQ(l.at(-1, 0), 0);
    EXPECT_EQ(l.at(4, 0), 0);
    EXPECT_EQ(l.at(0, 3), 0);
}

// ── Tilemap coordinate conversion + collision ────────────────────────────────

TEST(Tilemap, WorldToTileFloors) {
    Tilemap m(32, 32, 10, 10);
    Vec2 t = m.world_to_tile({70.f, 33.f});
    EXPECT_FLOAT_EQ(t.x, 2.f); // 70/32 = 2.18
    EXPECT_FLOAT_EQ(t.y, 1.f);
}

TEST(Tilemap, TileToWorldIsTopLeft) {
    Tilemap m(32, 32, 10, 10);
    Vec2 w = m.tile_to_world(3, 2);
    EXPECT_FLOAT_EQ(w.x, 96.f);
    EXPECT_FLOAT_EQ(w.y, 64.f);
}

TEST(Tilemap, CoordConversionHonoursNodePosition) {
    Tilemap m(32, 32, 10, 10);
    m.set_position(100.f, 0.f);
    Vec2 t = m.world_to_tile({132.f, 0.f}); // 32 px into the map
    EXPECT_FLOAT_EQ(t.x, 1.f);
    EXPECT_FLOAT_EQ(m.tile_to_world(1, 0).x, 132.f);
}

TEST(Tilemap, SolidGridSetAndQuery) {
    Tilemap m(32, 32, 5, 5);
    EXPECT_FALSE(m.is_solid(2, 2));
    m.set_solid(2, 2, true);
    EXPECT_TRUE(m.is_solid(2, 2));
    EXPECT_FALSE(m.is_solid(99, 99)); // out of bounds is non-solid
}

TEST(Tilemap, CollisionFromLayerMarksNonzeroTiles) {
    Tilemap m(32, 32, 4, 4);
    auto l = m.add_layer("walls");
    l->set(1, 1, 5);
    l->set(2, 1, 9);
    m.set_collision_from_layer(0);
    EXPECT_TRUE(m.is_solid(1, 1));
    EXPECT_TRUE(m.is_solid(2, 1));
    EXPECT_FALSE(m.is_solid(0, 0));
}

TEST(Tilemap, RectOverlapsSolid) {
    Tilemap m(32, 32, 10, 10);
    m.set_solid(3, 3, true); // world rect [96,96]..[128,128]
    EXPECT_TRUE(m.rect_overlaps_solid(Rect{100.f, 100.f, 10.f, 10.f}));
    EXPECT_FALSE(m.rect_overlaps_solid(Rect{0.f, 0.f, 10.f, 10.f}));
}

TEST(Tilemap, RectTouchingTileBoundaryDoesNotFalseCollide) {
    Tilemap m(32, 32, 10, 10);
    m.set_solid(3, 3, true);
    // A rect ending exactly at the solid tile's left edge (x=96) must not hit.
    EXPECT_FALSE(m.rect_overlaps_solid(Rect{64.f, 96.f, 32.f, 32.f}));
}

TEST(Tilemap, LayerLookupByNameAndIndex) {
    Tilemap m(16, 16, 2, 2);
    m.add_layer("ground");
    m.add_layer("overhead");
    EXPECT_EQ(m.layer(0)->name, "ground");
    EXPECT_EQ(m.layer_by_name("overhead")->name, "overhead");
    EXPECT_EQ(m.layer(99), nullptr);
    EXPECT_EQ(m.layer_by_name("nope"), nullptr);
}

TEST(Tilemap, TilesetForGidPicksHighestFirstGid) {
    Tilemap m(16, 16, 2, 2);
    m.add_tileset(nullptr, 16, 16, 1);
    m.add_tileset(nullptr, 16, 16, 100);
    EXPECT_EQ(m.tileset_for_gid(5)->first_gid, 1);
    EXPECT_EQ(m.tileset_for_gid(150)->first_gid, 100);
}

// ── TMX parsing (pure, no GPU) ───────────────────────────────────────────────

static const char* kTmx =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<map version=\"1.10\" orientation=\"orthogonal\" width=\"3\" height=\"2\""
    " tilewidth=\"32\" tileheight=\"32\">\n"
    " <tileset firstgid=\"1\" name=\"tiles\" tilewidth=\"32\" tileheight=\"32\""
    " tilecount=\"4\" columns=\"2\">\n"
    "  <image source=\"tiles.png\" width=\"64\" height=\"64\"/>\n"
    " </tileset>\n"
    " <layer name=\"ground\" width=\"3\" height=\"2\">\n"
    "  <data encoding=\"csv\">\n"
    "1,2,3,\n"
    "4,0,1\n"
    "  </data>\n"
    " </layer>\n"
    "</map>\n";

TEST(ParseTmx, MapDimensions) {
    TmxMap m = parse_tmx(kTmx);
    EXPECT_EQ(m.width, 3);
    EXPECT_EQ(m.height, 2);
    EXPECT_EQ(m.tile_w, 32);
    EXPECT_EQ(m.tile_h, 32);
}

TEST(ParseTmx, EmbeddedTileset) {
    TmxMap m = parse_tmx(kTmx);
    ASSERT_EQ(m.tilesets.size(), 1u);
    const TmxTileset& t = m.tilesets[0];
    EXPECT_EQ(t.first_gid, 1);
    EXPECT_EQ(t.tile_w, 32);
    EXPECT_EQ(t.columns, 2);
    EXPECT_EQ(t.image, "tiles.png");
    EXPECT_EQ(t.image_w, 64);
    EXPECT_TRUE(t.source.empty());
}

TEST(ParseTmx, LayerCsvData) {
    TmxMap m = parse_tmx(kTmx);
    ASSERT_EQ(m.layers.size(), 1u);
    const TmxLayer& l = m.layers[0];
    EXPECT_EQ(l.name, "ground");
    EXPECT_EQ(l.width, 3);
    EXPECT_EQ(l.height, 2);
    ASSERT_EQ(l.gids.size(), 6u);
    EXPECT_EQ(l.gids[0], 1);
    EXPECT_EQ(l.gids[2], 3);
    EXPECT_EQ(l.gids[3], 4);
    EXPECT_EQ(l.gids[4], 0);
}

TEST(ParseTmx, AttrDoesNotConfuseWidthWithTilewidth) {
    TmxMap m = parse_tmx(kTmx);
    EXPECT_EQ(m.width, 3);   // not 32 (tilewidth)
    EXPECT_EQ(m.tile_w, 32);
}

TEST(ParseTmx, StripsFlipFlags) {
    // gid with the horizontal-flip flag (bit 31) set must mask down to 5.
    const char* xml =
        "<map width=\"1\" height=\"1\" tilewidth=\"16\" tileheight=\"16\">"
        "<layer name=\"l\" width=\"1\" height=\"1\">"
        "<data encoding=\"csv\">2147483653</data></layer></map>"; // 0x80000005
    TmxMap m = parse_tmx(xml);
    ASSERT_EQ(m.layers.size(), 1u);
    ASSERT_EQ(m.layers[0].gids.size(), 1u);
    EXPECT_EQ(m.layers[0].gids[0], 5);
}

TEST(ParseTmx, RejectsNonCsvEncoding) {
    const char* xml =
        "<map width=\"1\" height=\"1\" tilewidth=\"16\" tileheight=\"16\">"
        "<layer name=\"l\" width=\"1\" height=\"1\">"
        "<data encoding=\"base64\">abcd</data></layer></map>";
    EXPECT_THROW(parse_tmx(xml), std::runtime_error);
}

TEST(ParseTmx, ExternalTilesetReference) {
    const char* xml =
        "<map width=\"2\" height=\"2\" tilewidth=\"16\" tileheight=\"16\">"
        "<tileset firstgid=\"1\" source=\"world.tsx\"/>"
        "<layer name=\"l\" width=\"2\" height=\"2\">"
        "<data encoding=\"csv\">0,0,0,0</data></layer></map>";
    TmxMap m = parse_tmx(xml);
    ASSERT_EQ(m.tilesets.size(), 1u);
    EXPECT_EQ(m.tilesets[0].source, "world.tsx");
    EXPECT_EQ(m.tilesets[0].first_gid, 1);
}

TEST(ParseTsx, ReadsTilesetAndImage) {
    const char* xml =
        "<?xml version=\"1.0\"?>"
        "<tileset name=\"world\" tilewidth=\"24\" tileheight=\"24\""
        " tilecount=\"9\" columns=\"3\" margin=\"1\" spacing=\"2\">"
        "<image source=\"world.png\" width=\"76\" height=\"76\"/></tileset>";
    TmxTileset t = parse_tsx(xml);
    EXPECT_EQ(t.tile_w, 24);
    EXPECT_EQ(t.columns, 3);
    EXPECT_EQ(t.margin, 1);
    EXPECT_EQ(t.spacing, 2);
    EXPECT_EQ(t.image, "world.png");
    EXPECT_EQ(t.image_w, 76);
}
