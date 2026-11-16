#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "tree/bTree.hpp"
#include "tree/multiwayTree.hpp"

namespace {

template <std::size_t degree>
void verifySequence(const std::vector<int>& input) {
    general_functions::BTree<int, degree> tree;
    std::multiset<int> reference;
    ASSERT_TRUE(tree.empty());
    ASSERT_TRUE(tree.validateInvariants());
    for (int key : input) {
        tree.insert(key);
        reference.insert(key);
        ASSERT_EQ(tree.size(), reference.size());
        ASSERT_TRUE(tree.contains(key));
        ASSERT_TRUE(tree.validateInvariants()) << "after inserting " << key;
    }
    for (int key : reference) {
        EXPECT_TRUE(tree.contains(key));
    }
    EXPECT_FALSE(tree.contains(std::numeric_limits<int>::min()));
    EXPECT_FALSE(tree.contains(std::numeric_limits<int>::max()));
    auto copy = tree;
    copy.insert(std::numeric_limits<int>::min());
    EXPECT_EQ(tree.size(), reference.size());
    EXPECT_EQ(copy.size(), reference.size() + 1);
    EXPECT_FALSE(tree.contains(std::numeric_limits<int>::min()));
    EXPECT_TRUE(copy.validateInvariants());
}

template <std::size_t degree>
void verifyOrders() {
    std::vector<int> keys(1000);
    std::iota(keys.begin(), keys.end(), -500);
    verifySequence<degree>(keys);
    std::reverse(keys.begin(), keys.end());
    verifySequence<degree>(keys);
    std::mt19937 random(42);
    std::shuffle(keys.begin(), keys.end(), random);
    verifySequence<degree>(keys);
    keys.assign(600, 7);
    verifySequence<degree>(keys);
    std::uniform_int_distribution<int> distribution(-30, 30);
    for (auto& key : keys) {
        key = distribution(random);
    }
    verifySequence<degree>(keys);
}

TEST(BTreeTest, DegreeTwo) {
    verifyOrders<2>();
}
TEST(BTreeTest, DegreeThree) {
    verifyOrders<3>();
}
TEST(BTreeTest, DegreeEight) {
    verifyOrders<8>();
}
TEST(BTreeTest, DegreeThirtyTwo) {
    verifyOrders<32>();
}

TEST(BTreeTest, IntegerBoundaries) {
    general_functions::BTree<int, 2> tree;
    for (int index = 0; index < 100; ++index) {
        tree.insert(std::numeric_limits<int>::min());
        tree.insert(std::numeric_limits<int>::max());
        tree.insert(0);
        ASSERT_TRUE(tree.validateInvariants());
    }
    EXPECT_EQ(tree.size(), 300U);
    EXPECT_TRUE(tree.contains(std::numeric_limits<int>::min()));
    EXPECT_TRUE(tree.contains(std::numeric_limits<int>::max()));
    EXPECT_FALSE(tree.contains(1));
}

struct Record {
    explicit Record(int data, int sequence = 0) : value(data), tag(sequence) {}
    int value;
    int tag;
};

struct ByValue {
    bool operator()(const Record& left, const Record& right) const {
        return left.value < right.value;
    }
};

TEST(BTreeTest, ComparatorEquivalenceWithoutEqualityOperator) {
    general_functions::BTree<Record, 2, ByValue> tree;
    for (int index = 0; index < 300; ++index) {
        tree.insert(Record(index % 7, index));
        ASSERT_TRUE(tree.validateInvariants());
    }
    EXPECT_EQ(tree.size(), 300U);
    for (int value = 0; value < 7; ++value) {
        EXPECT_TRUE(tree.contains(Record(value, -1)));
    }
    EXPECT_FALSE(tree.contains(Record(8)));
}

TEST(BTreeTest, DescendingOrder) {
    general_functions::BTree<int, 3, std::greater<int>> tree;
    for (int key = 0; key < 1000; ++key) {
        tree.insert(key % 137);
        ASSERT_TRUE(tree.validateInvariants());
    }
    EXPECT_EQ(tree.size(), 1000U);
    for (int key = 0; key < 137; ++key) {
        EXPECT_TRUE(tree.contains(key));
    }
    EXPECT_FALSE(tree.contains(200));
}

struct ThrowingKey {
    explicit ThrowingKey(int data) : value(data) {}
    ThrowingKey(const ThrowingKey& other) : value(other.value) { checkCopy(); }
    ThrowingKey(ThrowingKey&&) noexcept = default;
    ThrowingKey& operator=(const ThrowingKey& other) {
        checkCopy();
        value = other.value;
        return *this;
    }
    ThrowingKey& operator=(ThrowingKey&&) noexcept = default;
    static void checkCopy() {
        if (copiesBeforeThrow == 0) {
            throw std::runtime_error("Injected key copy failure");
        }
        if (copiesBeforeThrow > 0) {
            --copiesBeforeThrow;
        }
    }
    int value;
    inline static int copiesBeforeThrow = -1;
};

struct ThrowingKeyLess {
    bool operator()(const ThrowingKey& left, const ThrowingKey& right) const {
        return left.value < right.value;
    }
};

TEST(BTreeTest, FailedCopyDuringInsertionPreservesContentsAndStructure) {
    using Tree = general_functions::BTree<ThrowingKey, 2, ThrowingKeyLess>;
    Tree original;
    for (int key = 0; key < 67; ++key) {
        original.insert(ThrowingKey(key));
    }
    int failures = 0;
    int successes = 0;
    for (int budget = 0; budget < 100; ++budget) {
        ThrowingKey::copiesBeforeThrow = -1;
        Tree tree(original);
        ThrowingKey::copiesBeforeThrow = budget;
        bool inserted = false;
        try {
            tree.insert(ThrowingKey(67));
            inserted = true;
            ++successes;
        } catch (const std::runtime_error&) {
            ++failures;
        }
        ThrowingKey::copiesBeforeThrow = -1;
        ASSERT_TRUE(tree.validateInvariants()) << "copy budget " << budget;
        ASSERT_EQ(tree.size(), inserted ? 68U : 67U);
        for (int key = 0; key < 67; ++key) {
            ASSERT_TRUE(tree.contains(ThrowingKey(key)));
        }
        ASSERT_EQ(tree.contains(ThrowingKey(67)), inserted);
    }
    EXPECT_GT(failures, 0);
    EXPECT_GT(successes, 0);
}

struct ThrowingCompare {
    bool operator()(int left, int right) const {
        if (comparisonsBeforeThrow == 0) {
            throw std::runtime_error("Injected comparator failure");
        }
        if (comparisonsBeforeThrow > 0) {
            --comparisonsBeforeThrow;
        }
        return left < right;
    }
    inline static int comparisonsBeforeThrow = -1;
};

TEST(BTreeTest, FailedComparisonPreservesContentsAndStructure) {
    using Tree = general_functions::BTree<int, 2, ThrowingCompare>;
    Tree original;
    for (int key = 0; key < 67; ++key) {
        original.insert(key);
    }
    for (int budget = 0; budget < 30; ++budget) {
        Tree tree(original);
        ThrowingCompare::comparisonsBeforeThrow = budget;
        bool inserted = false;
        try {
            tree.insert(67);
            inserted = true;
        } catch (const std::runtime_error&) {
        }
        ThrowingCompare::comparisonsBeforeThrow = -1;
        ASSERT_TRUE(tree.validateInvariants());
        EXPECT_EQ(tree.size(), inserted ? 68U : 67U);
        EXPECT_EQ(tree.contains(67), inserted);
        for (int key = 0; key < 67; ++key) {
            ASSERT_TRUE(tree.contains(key));
        }
    }
}

} // namespace
