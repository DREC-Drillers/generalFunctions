#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "tree/avlTree.hpp"

namespace {
using Trees = ::testing::Types<general_functions::AVLTree<int>>;
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
    if constexpr (std::is_same_v<Tree, general_functions::AVLTree<int>>) {
        expected.erase(std::unique(expected.begin(), expected.end()),
                       expected.end());
    }
    return expected;
}

TYPED_TEST(SearchTreeAPITest, EmptyTreeAndInvalidPositions) {
    TypeParam tree;
    EXPECT_EQ(tree.lower_bound(0), tree.end());
    EXPECT_EQ(tree.upper_bound(0), tree.end());
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
        EXPECT_EQ(first, std::next(tree.begin(), lower - expected.begin()));
        EXPECT_EQ(last, std::next(tree.begin(), upper - expected.begin()));
        EXPECT_EQ(tree.rank(key),
                  static_cast<std::size_t>(lower - expected.begin()));
        if (lower != expected.end()) {
            ASSERT_NE(first, tree.end());
            EXPECT_EQ(*first, *lower);
        }
        if (upper != expected.end()) {
            ASSERT_NE(last, tree.end());
            EXPECT_EQ(*last, *upper);
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
    const auto& view = tree;
    EXPECT_EQ(view.lower_bound(MIN_KEY), view.begin());
    EXPECT_EQ(view.upper_bound(MIN_KEY), std::next(view.begin()));
    EXPECT_EQ(view.lower_bound(MAX_KEY), std::prev(view.end()));
    EXPECT_EQ(view.upper_bound(MAX_KEY), view.end());
    static_assert(std::is_same_v<decltype(*view.lower_bound(0)), const int&>);
}

} // namespace
