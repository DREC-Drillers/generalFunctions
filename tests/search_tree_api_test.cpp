#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <set>
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
    EXPECT_EQ(tree.begin(), tree.end());
    EXPECT_EQ(tree.lower_bound(0), tree.end());
    EXPECT_EQ(tree.upper_bound(0), tree.end());
    EXPECT_EQ(tree.distance(tree.begin(), tree.end()), 0);
    EXPECT_THROW(tree.select(0), std::out_of_range);
    EXPECT_THROW(*tree.end(), std::out_of_range);
    auto end = tree.end();
    EXPECT_THROW(++end, std::out_of_range);
    typename TypeParam::iterator singular;
    EXPECT_THROW(*singular, std::logic_error);
}

TYPED_TEST(SearchTreeAPITest, IterationAndSelectMatchSortedKeys) {
    TypeParam tree;
    const auto expected = populate(tree);
    const TypeParam& readOnly = tree;
    const std::vector<int> actual(readOnly.begin(), readOnly.end());
    EXPECT_EQ(actual, expected);
    EXPECT_EQ(std::distance(readOnly.begin(), readOnly.end()),
              static_cast<std::ptrdiff_t>(expected.size()));
    EXPECT_EQ(tree.distance(tree.begin(), tree.end()),
              static_cast<std::ptrdiff_t>(expected.size()));
    EXPECT_EQ(tree.distance(tree.end(), tree.begin()),
              -static_cast<std::ptrdiff_t>(expected.size()));
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tree.select(i), expected[i]);
    }
    EXPECT_THROW(tree.select(tree.size()), std::out_of_range);
    EXPECT_THROW(tree.select(std::numeric_limits<std::size_t>::max()),
                 std::out_of_range);
    auto it = tree.begin();
    EXPECT_EQ(*it++, expected[0]);
    EXPECT_EQ(*it, expected[1]);
    static_assert(std::is_same_v<decltype(*tree.begin()), const int&>);
}

TYPED_TEST(SearchTreeAPITest, BoundsMatchStandardAlgorithms) {
    TypeParam tree;
    const auto expected = populate(tree);
    for (int key = -55; key <= 55; ++key) {
        const auto lower = std::lower_bound(expected.begin(), expected.end(), key);
        const auto upper = std::upper_bound(expected.begin(), expected.end(), key);
        const auto first = tree.lower_bound(key);
        const auto last = tree.upper_bound(key);
        EXPECT_EQ(tree.distance(tree.begin(), first), lower - expected.begin());
        EXPECT_EQ(tree.distance(tree.begin(), last), upper - expected.begin());
        EXPECT_EQ(tree.distance(first, last), upper - lower);
        if (lower == expected.end()) {
            EXPECT_EQ(first, tree.end());
        } else {
            EXPECT_EQ(*first, *lower);
        }
        if (upper == expected.end()) {
            EXPECT_EQ(last, tree.end());
        } else {
            EXPECT_EQ(*last, *upper);
        }
    }
}

TYPED_TEST(SearchTreeAPITest, RejectsForeignIterators) {
    TypeParam tree;
    TypeParam other;
    tree.insert(1);
    other.insert(1);
    EXPECT_NE(tree.begin(), other.begin());
    EXPECT_THROW(tree.distance(tree.begin(), other.end()), std::invalid_argument);
    EXPECT_THROW(tree.distance(other.begin(), tree.end()), std::invalid_argument);
    typename TypeParam::iterator singular;
    EXPECT_THROW(tree.distance(singular, tree.end()), std::invalid_argument);
}

TYPED_TEST(SearchTreeAPITest, ExtremeKeys) {
    TypeParam tree;
    constexpr int MIN_KEY = std::numeric_limits<int>::min();
    constexpr int MAX_KEY = std::numeric_limits<int>::max();
    for (int key : {0, MIN_KEY, MAX_KEY}) {
        tree.insert(key);
    }
    EXPECT_EQ(*tree.lower_bound(MIN_KEY), MIN_KEY);
    EXPECT_EQ(*tree.upper_bound(MIN_KEY), 0);
    EXPECT_EQ(*tree.lower_bound(MAX_KEY), MAX_KEY);
    EXPECT_EQ(tree.upper_bound(MAX_KEY), tree.end());
    EXPECT_EQ(tree.distance(tree.upper_bound(MIN_KEY), tree.upper_bound(MAX_KEY)),
              2);
}

} // namespace
