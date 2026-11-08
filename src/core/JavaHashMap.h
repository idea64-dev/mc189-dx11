#pragma once

// java.util.HashMap's iteration order. Where the original iterates a HashMap and the order shows (NBT compound
// files and toString, ...), the port has to visit entries in the same order, which depends on the keys' Java
// hashCode, the table size and the insertion history. This reproduces Java 8's bucket layout: table sizes
// growing 16, 32, 64... at a 0.75 load factor, hash spreading (h ^ h >>> 16), new entries appended to their
// bucket, resizes splitting buckets in order, and the early resize when a bucket of a small table reaches 9
// entries. Not reproduced: the reordering inside treeified buckets (9+ colliding keys in a table of 64+).
//
// Lookups are linear within a bucket, as in Java; that's fine for the small maps this is used for.

#include "core/Java.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// Java hashCode of a key type.
template <class K>
struct JavaHash;

template <>
struct JavaHash<std::string> {
    int32_t operator()(const std::string& key) const { return java::hashCode(key); }
};

template <class K, class V, class Hash = JavaHash<K>>
class JavaHashMap {
public:
    using value_type = std::pair<K, V>;

    class const_iterator {
    public:
        const value_type& operator*() const { return map->table[bucket][index].entry; }
        const value_type* operator->() const { return &**this; }
        const_iterator& operator++()
        {
            ++index;
            skip();
            return *this;
        }
        bool operator==(const const_iterator& other) const { return bucket == other.bucket && index == other.index; }

    private:
        friend class JavaHashMap;
        const_iterator(const JavaHashMap* m, size_t b, size_t i)
            : map(m)
            , bucket(b)
            , index(i)
        {
            skip();
        }
        void skip()
        {
            while (bucket < map->table.size() && index >= map->table[bucket].size()) {
                ++bucket;
                index = 0;
            }
        }
        const JavaHashMap* map;
        size_t bucket;
        size_t index;
    };

    const_iterator begin() const { return const_iterator(this, 0, 0); }
    const_iterator end() const { return const_iterator(this, table.size(), 0); }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }

    void clear()
    {
        // Java's clear() empties the buckets but keeps the table size.
        for (auto& bucket : table)
            bucket.clear();
        count = 0;
    }

    const V* find(const K& key) const
    {
        if (table.empty())
            return nullptr;
        for (const Node& node : table[indexFor(spread(key))]) {
            if (node.entry.first == key)
                return &node.entry.second;
        }
        return nullptr;
    }

    V* find(const K& key) { return const_cast<V*>(static_cast<const JavaHashMap*>(this)->find(key)); }
    bool contains(const K& key) const { return find(key) != nullptr; }

    // HashMap.put: replaces the value in place or appends a new entry. Returns true if the key was new.
    bool put(const K& key, V value)
    {
        if (table.empty())
            resize();
        const int32_t hash = spread(key);
        auto& bucket = table[indexFor(hash)];
        for (Node& node : bucket) {
            if (node.hash == hash && node.entry.first == key) {
                node.entry.second = std::move(value);
                return false;
            }
        }
        bucket.push_back(Node { hash, { key, std::move(value) } });
        if (bucket.size() >= 9 && table.size() < 64)
            resize(); // treeifyBin on a small table
        if (++count > threshold)
            resize();
        return true;
    }

    bool erase(const K& key)
    {
        if (table.empty())
            return false;
        auto& bucket = table[indexFor(spread(key))];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->entry.first == key) {
                bucket.erase(it);
                --count;
                return true;
            }
        }
        return false;
    }

private:
    struct Node {
        int32_t hash;
        value_type entry;
    };

    static int32_t spread(const K& key)
    {
        const int32_t h = Hash {}(key);
        return h ^ java::ushr(h, 16);
    }

    size_t indexFor(int32_t hash) const { return static_cast<size_t>(static_cast<uint32_t>(hash) & (table.size() - 1)); }

    void resize()
    {
        const size_t newSize = table.empty() ? 16 : table.size() * 2;
        std::vector<std::vector<Node>> newTable(newSize);
        for (auto& bucket : table) {
            for (Node& node : bucket)
                newTable[static_cast<size_t>(static_cast<uint32_t>(node.hash) & (newSize - 1))].push_back(std::move(node));
        }
        table = std::move(newTable);
        threshold = newSize * 3 / 4;
    }

    std::vector<std::vector<Node>> table;
    size_t count = 0;
    size_t threshold = 0;
};

} // namespace mc
