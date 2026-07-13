#include <gtest/gtest.h>
#include "platform/paths.hpp"
#include <cstdio>
#include <fstream>

using namespace loom;

TEST(Paths, SaveDirIsAbsoluteAndEndsWithASeparator) {
    const std::string dir = save_dir("loom2d", "tests");
    ASSERT_FALSE(dir.empty());

    // Callers join a filename straight onto it, so the trailing separator that
    // SDL guarantees is part of the contract.
    const char last = dir.back();
    EXPECT_TRUE(last == '/' || last == '\\') << dir;
}

TEST(Paths, SaveDirIsActuallyWritable) {
    // The directory is created for us — the point of asking the OS rather than
    // writing next to the executable, which is often read-only once installed.
    const std::string path = save_dir("loom2d", "tests") + "write_probe.txt";

    {
        std::ofstream out(path);
        ASSERT_TRUE(out.is_open()) << path;
        out << "loom2d";
    }
    {
        std::ifstream in(path);
        std::string content;
        in >> content;
        EXPECT_EQ(content, "loom2d");
    }
    std::remove(path.c_str());
}

TEST(Paths, DifferentAppsGetDifferentDirectories) {
    EXPECT_NE(save_dir("loom2d", "tests"), save_dir("loom2d", "tests_other"));
}
