#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "tree/avlTree.hpp"

namespace {
using Trees = ::testing::Types<general_functions::AVLTree>;
template <typename Tree>
class SearchTreeAPITest : public ::testing::Test {};
TYPED_TEST_SUITE(SearchTreeAPITest, Trees, );

template <typename Tree>
std::vector<int> populate(Tree& tree) {
    std::vector<int> expected;
    for (int i = 0; i < 400; ++i) {
        const int key = (i * 37) % 101 - 50;
        tree.insert(key);
        expected.push_back(key);
    }
    std::sort(expected.begin(), expected.end());
    if constexpr (std::is_same_v<Tree, general_functions::AVLTree>) {
        expected.erase(std::unique(expected.begin(), expected.end()),
                       expected.end());
    }
    return expected;
}

TYPED_TEST(SearchTreeAPITest, EmptyTreeAndInvalidPositions) {
    TypeParam tree;
    EXPECT_EQ(tree.lower_bound(0), 0U);
    EXPECT_EQ(tree.upper_bound(0), 0U);
    EXPECT_THROW(tree.select(0), std::out_of_range);
}

TYPED_TEST(SearchTreeAPITest, SelectMatchesSortedKeys) {
    TypeParam tree;
    const auto expected = populate(tree);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tree.select(i), expected[i]);
    }
    EXPECT_THROW(tree.select(tree.size()), std::out_of_range);
    EXPECT_THROW(tree.select(std::numeric_limits<std::size_t>::max()),
                 std::out_of_range);
    static_assert(std::is_same_v<decltype(tree.select(0)), const int&>);
}

TYPED_TEST(SearchTreeAPITest, BoundsMatchStandardAlgorithms) {
    TypeParam tree;
    const auto expected = populate(tree);
    for (int key = -55; key <= 55; ++key) {
        const auto lower = std::lower_bound(expected.begin(), expected.end(), key);
        const auto upper = std::upper_bound(expected.begin(), expected.end(), key);
        const auto first = tree.lower_bound(key);
        const auto last = tree.upper_bound(key);
        EXPECT_EQ(first, static_cast<std::size_t>(lower - expected.begin()));
        EXPECT_EQ(last, static_cast<std::size_t>(upper - expected.begin()));
        EXPECT_EQ(last - first, static_cast<std::size_t>(upper - lower));
        if (first < tree.size()) {
            EXPECT_EQ(tree.select(first), *lower);
        }
        if (last < tree.size()) {
            EXPECT_EQ(tree.select(last), *upper);
        }
    }
}

TYPED_TEST(SearchTreeAPITest, ExtremeKeys) {
    TypeParam tree;
    constexpr int MIN_KEY = std::numeric_limits<int>::min();
    constexpr int MAX_KEY = std::numeric_limits<int>::max();
    for (int key : {0, MIN_KEY, MAX_KEY}) {
        tree.insert(key);
    }
    EXPECT_EQ(tree.lower_bound(MIN_KEY), 0U);
    EXPECT_EQ(tree.upper_bound(MIN_KEY), 1U);
    EXPECT_EQ(tree.lower_bound(MAX_KEY), 2U);
    EXPECT_EQ(tree.upper_bound(MAX_KEY), tree.size());
}

} // namespace
