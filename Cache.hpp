#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <stdint.h>
#include <list>
#include <map>
#include <memory>

namespace graph_tools {
    namespace memory_modeling {

        class CacheBuilder {
        public:
            using Addr = intptr_t;

        private:
            Addr _size;
            Addr _block_size;
            Addr _assoc;
        public:
            CacheBuilder(Addr size,
                         Addr block_size,
                         Addr assoc = 2):
                _size(size),
                _block_size(block_size),
                _assoc(assoc) {
            }
            Addr size() const { return _size; }
            Addr block_size() const { return _block_size; }
            Addr assoc() const { return _assoc; }
        };

        class Cache {
        public:
            static constexpr bool MISS = true;
            static constexpr bool HIT  = false;
            using Ptr       = std::shared_ptr<Cache>;
            using CPtr      = std::shared_ptr<const Cache>;
            using Set       = intptr_t;
            using Way       = intptr_t;
            using Addr      = intptr_t;
            using Tag       = intptr_t;

            virtual ~Cache() {};

            Cache(Addr size, Addr block_size, Addr assoc = 2) :
                _assoc(assoc),
                _block_size(block_size),
                _cold_misses(0) {

                Addr sets = size/(_block_size * _assoc);
                for (Addr i = 0; i < sets; i++)
                    _sets.push_back(SetInternal(_assoc));
            }

            Cache(const CacheBuilder & builder) :
                Cache(builder.size(), builder.block_size(), builder.assoc()) {
            }

            /* primary api */
            void load(Addr addr) { access(addr, false); }
            void load_multi(Addr addr, Addr sz) {
                while (sz > 0) {
                    load(addr);
                    auto ldsz = block_remainder_from_addr(addr);
                    addr += ldsz;
                    sz   -= ldsz;
                }
            }

            void store(Addr addr) { access(addr, true); }
            void store_multi(Addr addr, Addr sz) {
                while (sz > 0) {
                    store(addr);
                    auto stsz = block_remainder_from_addr(addr);
                    addr += stsz;
                    sz   -= stsz;
                }
            }

            Addr size() const  { return sets() * assoc() * block_size(); }
            Addr sets() const  { return _sets.size(); }
            Addr assoc() const { return _assoc; }
            Addr block_size() const { return _block_size; }

            /* stats api */
            std::string parameters_csv_header() const {
                return "size,assoc,sets,block_size";
            }

            std::string parameters_csv() const {
                std::stringstream ss;
                ss << size() << ",";
                ss << assoc() << ",";
                ss << sets() << ",";
                ss << block_size();
                return ss.str();
            }

            std::string stats_csv_header() const {
                return "hits,misses,cold_misses,flushes";
            }
            std::string stats_csv() const {
                std::stringstream ss;
                ss << sum_hits() << ",";
                ss << sum_misses() << ",";
                ss << compulsory_misses() << ",";
                ss << sum_flushes() << ",";
                return ss.str();
            }

        protected:

            struct TagInfo {
                TagInfo(Tag tag = 0, bool valid = false, bool dirty = false) :
                    tag(tag), valid(valid), dirty(dirty) {}

                Tag  tag;
                bool valid;
                bool dirty;
            };

            using SetInternal = std::vector<TagInfo>;

            /* policy subclassing api */
            virtual Way  eject_one(Set set) = 0;
            virtual void use(Set set, Way way) = 0;

            std::vector< SetInternal > _sets;

        private:
            void access(Addr addr, bool store) {
                Tag tag = tag_from_addr(addr);
                Set set_id = set_from_addr(addr);

                // search for a hit
                auto &set = _sets[set_id];
                for (auto way = 0; way < assoc(); way++) {
                    auto & tag_info = set[way];
                    // hit
                    if (tag == tag_info.tag &&
                        tag_info.valid) {
                        tag_info.dirty |= store;
                        record_hit(addr);
                        use(set_id, way);
                        return;
                    }
                }
                // miss
                record_miss(addr);

                // search for an invalid way
                for (auto way = 0; way < assoc(); way++) {
                    auto &tag_info = set[way];
                    if (!tag_info.valid) {
                        tag_info.valid = true;
                        tag_info.tag = tag;
                        tag_info.dirty = store;
                        use(set_id, way);
                        return;
                    }
                }

                // conflict miss; eject one
                auto way = eject_one(set_id);
                auto &tag_info = set[way];

                if (tag_info.dirty) record_flush(addr_from_set_and_tag(set_id, tag_info.tag));

                tag_info.valid = true;
                tag_info.tag = tag;
                tag_info.dirty = store;
                use(set_id, way);
                return;
            }

            /* compute the remaining bytes in block from address */
            Addr block_remainder_from_addr(Addr addr) const {
                return block_size() - block_offset_from_addr(addr);
            }

            /* compute offset in block from address */
            Addr block_offset_from_addr(Addr addr) const {
                return addr % block_size();
            }

            /* compute block address without block offset */
            Addr block_addr_from_addr(Addr addr) const {
                return addr - block_offset_from_addr(addr);
            }

            /* compute tag data from addr */
            Tag tag_from_addr(Addr addr) const {
                return (addr/block_size()) / sets();
            }

            /* compute set index from addr */
            Set set_from_addr(Addr addr) const {
                return (addr/block_size()) % sets();
            }

            /* compute the block id from the address */
            Addr block_from_addr(Addr addr) const {
                return addr / block_size();
            }

            /* compute the block id from the set and tag */
            Addr block_from_set_and_tag(Set set, Tag tag) const {
                return (tag * sets()) + set;
            }

            /* compute the address from the set and tag */
            Addr addr_from_set_and_tag(Set set, Tag tag) const {
                return block_from_set_and_tag(set, tag) * block_size();
            }

        private:
            Addr _assoc;
            Addr _block_size;

            struct Stats {
                Stats(int64_t hits = 0, int64_t misses = 0, int64_t flushes = 0) :
                    hits(hits), misses(misses), flushes(flushes) {}
                int64_t hits;
                int64_t misses;
                int64_t flushes;
            };

            std::map<Addr, Stats> _stats;
            int64_t _cold_misses;

            Stats & find_stat(Addr addr) {
                auto p = _stats.find(addr);
                if (p != _stats.end()) {
                    return p->second;
                } else {
                    _stats[addr] = Stats();
                    return _stats[addr];
                }

            }

            bool is_cold_miss(Addr addr) const {
                auto p = _stats.find(block_from_addr(addr));
                return p == _stats.end();
            }

            void record_hit(Addr addr) {
                auto &stat = find_stat(block_from_addr(addr));
                stat.hits++;
            }

            void record_miss(Addr addr) {
                if (is_cold_miss(addr)) _cold_misses++;
                auto &stat = find_stat(block_from_addr(addr));
                stat.misses++;
            }

            void record_flush(Addr addr) {
                auto &stat = find_stat(block_from_addr(addr));
                stat.flushes++;
            }

        public:
            /* stats api */
            int64_t compulsory_misses() const {
                return _cold_misses;
            }

            int64_t sum_hits(void) const {
                int64_t sum = 0;
                for (auto &p : _stats)
                    sum += p.second.hits;

                return sum;
            }

            int64_t sum_misses(void) const {
                int64_t sum = 0;
                for (auto &p : _stats)
                    sum += p.second.misses;
                return sum;
            }

            int64_t sum_flushes(void) const {
                int64_t sum = 0;
                for (auto &p : _stats)
                    sum += p.second.flushes;
                return sum;
            }

        };

        class LMRUCacheBase : public Cache {
        public:
            LMRUCacheBase(Addr size, Addr block_size, Addr assoc = 2) :
                Cache(size, block_size, assoc),
                _used(sets()) {
            }

            LMRUCacheBase(const CacheBuilder & builder) :
                LMRUCacheBase(builder.size(), builder.block_size(), builder.assoc()) {
            }

            void use(Set set, Way way) override {
                auto & tgt_set = _used[set];
                tgt_set.remove(way);
                // mru -> lru
                tgt_set.push_front(way);
            };

        protected:
            using UsedInternal = std::list<Way>;
            std::vector<UsedInternal> _used;
        };

        class MRUCache final : public LMRUCacheBase {
        public:
            MRUCache() : MRUCache(1,1,1) {}
            MRUCache(Addr size, Addr block_size, Addr assoc = 2) :
                LMRUCacheBase(size, block_size, assoc) {}
            MRUCache(const CacheBuilder & builder) :
                MRUCache(builder.size(), builder.block_size(), builder.assoc()) {
            }

        protected:
            Way eject_one(Set set) override {
                auto & tgt_set = _used[set];
                // mru is in the front
                auto r = tgt_set.front();
                tgt_set.pop_front();
                return r;
            }
        };

        class LRUCache final : public LMRUCacheBase {
        public:
            LRUCache() : LRUCache(1,1,1) {}
            LRUCache(Addr size, Addr block_size, Addr assoc = 2) :
                LMRUCacheBase(size, block_size, assoc) {}
            LRUCache(const CacheBuilder & builder) :
                LRUCache(builder.size(), builder.block_size(), builder.assoc()) {
            }
        protected:
            Way eject_one(Set set) override {
                auto & tgt_set = _used[set];
                // lru is in the back
                auto r = tgt_set.back();
                tgt_set.pop_back();
                return r;
            }
        };

        static Cache::Ptr HammerBladeCache() {
            int x = 16;
            int y = 8;
            int caches = x * 2; // top n bottom
            int sets = 64;
            int ways = 8;
            int block_size = 8*4; // 8 4-byte words
            return std::make_shared<LRUCache>(caches * sets * ways * block_size,
                                              block_size,
                                              ways);
        }

        static Cache::Ptr HammerBladeCache16() {
            int x = 16;
            int y = 8;
            int caches = x * 2; // top n bottom
            int sets = 64;
            int ways = 4;
            int block_size = 16*4; // 16 4-byte words
            return std::make_shared<LRUCache>(caches * sets * ways * block_size,
                                              block_size,
                                              ways);
        }
    }
}
