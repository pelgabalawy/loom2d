#include <gtest/gtest.h>
#include "assets/asset_manager.hpp"
#include <string>
#include <memory>

using namespace loom;

// Minimal fake asset for testing the generic cache logic
struct FakeAsset {
    std::string path;
    int         load_count = 0;

    explicit FakeAsset(std::string p) : path(std::move(p)) {}
};

static int g_load_calls = 0;

static std::shared_ptr<FakeAsset> fake_loader(const std::string& path) {
    ++g_load_calls;
    return std::make_shared<FakeAsset>(path);
}

class AssetCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_load_calls = 0;
        cache = std::make_unique<AssetCache<FakeAsset>>(fake_loader);
    }
    std::unique_ptr<AssetCache<FakeAsset>> cache;
};

TEST_F(AssetCacheTest, FirstGetLoadsAsset) {
    auto a = cache->get("hero.png");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->path, "hero.png");
    EXPECT_EQ(g_load_calls, 1);
}

TEST_F(AssetCacheTest, SecondGetReturnsCachedInstance) {
    auto a1 = cache->get("hero.png");
    auto a2 = cache->get("hero.png");
    EXPECT_EQ(a1.get(), a2.get()); // same object
    EXPECT_EQ(g_load_calls, 1);    // only loaded once
}

TEST_F(AssetCacheTest, DifferentPathsLoadSeparately) {
    auto a = cache->get("hero.png");
    auto b = cache->get("enemy.png");
    EXPECT_NE(a.get(), b.get());
    EXPECT_EQ(g_load_calls, 2);
}

TEST_F(AssetCacheTest, EvictedAssetReloadsOnNextGet) {
    {
        auto a = cache->get("tmp.png");
        EXPECT_EQ(g_load_calls, 1);
        // 'a' goes out of scope here — weak_ptr in cache becomes stale
    }
    auto b = cache->get("tmp.png"); // should reload
    EXPECT_EQ(g_load_calls, 2);
}

TEST_F(AssetCacheTest, ExplicitEvict) {
    auto a = cache->get("hero.png");
    cache->evict("hero.png");
    // Strong ref 'a' still alive; evict just clears the cache entry
    auto b = cache->get("hero.png"); // reloads
    EXPECT_EQ(g_load_calls, 2);
}

TEST_F(AssetCacheTest, ReloadForcesRefresh) {
    auto a1 = cache->get("hero.png");
    auto a2 = cache->reload("hero.png");
    EXPECT_EQ(g_load_calls, 2);
    // Reload returns a fresh pointer (new object)
    EXPECT_NE(a1.get(), a2.get());
}

TEST_F(AssetCacheTest, ClearRemovesAll) {
    cache->get("a.png");
    cache->get("b.png");
    cache->get("c.png");
    EXPECT_EQ(g_load_calls, 3);

    cache->clear();
    cache->get("a.png");
    EXPECT_EQ(g_load_calls, 4); // reloaded
}

TEST_F(AssetCacheTest, LiveCountTracksStrongRefs) {
    {
        auto a = cache->get("a.png");
        auto b = cache->get("b.png");
        EXPECT_EQ(cache->live_count(), 2);
    }
    // After scope, both refs dropped → live_count should show 0 (all expired)
    EXPECT_EQ(cache->live_count(), 0);
}
