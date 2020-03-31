#include <Cache.hpp>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <assert.h>

namespace graph_tools {
    namespace memory_modeling {
        template <typename T>
        class VectorWithCache {
            static_assert(std::is_scalar<T>::value,
                          "T must be a scalar value");

        public:
            VectorWithCache(std::shared_ptr<Cache> cache):
                _cache(cache) {
            }

            T get(size_t i) const {
                _cache->load_multi(reinterpret_cast<Cache::Addr>(&_data[i]),
                                   static_cast<Cache::Addr>(sizeof(T)));
                return _data[i];
            }

            void set(size_t i, T val) {
                _cache->store_multi(reinterpret_cast<Cache::Addr>(&_data[i]),
                                    static_cast<Cache::Addr>(sizeof(T)));
                _data[i] = val;
            }

            std::vector<T> & data() { return _data; }

            size_t size() const { return _data.size(); }

            std::shared_ptr<const Cache> cache() const {
                return static_cast<std::shared_ptr<const Cache>>(_cache);
            }

            /* Unit Testing */
            static int Test(int argc, char *argv[]) {
                // Make sure missing works
                {
                    // A cache that can hold a single element
                    std::shared_ptr<LRUCache> cache
                        = std::make_shared<LRUCache>(sizeof(T), sizeof(T), 1);

                    VectorWithCache<T> dram(cache);
                    dram.data() = {0, 1, 2, 3};

                    int x;
                    x = dram.get(0);
                    x = dram.get(1);
                    x = dram.get(0);

                    assert(dram.cache()->sum_hits() == 0);
                    assert(dram.cache()->sum_misses() == 3);
                    assert(dram.cache()->sum_flushes() == 0);
                    assert(dram.data()[0] == 0);
                    assert(dram.data()[1] == 1);
                    assert(dram.data()[2] == 2);
                    assert(dram.data()[3] == 3);
                }
                // Make sure hitting works
                {
                    // A cache that can hold a single element
                    std::shared_ptr<LRUCache> cache
                        = std::make_shared<LRUCache>(sizeof(T), sizeof(T), 1);

                    VectorWithCache<T> dram(cache);
                    dram.data() = {0, 1, 2, 3};

                    int x;
                    x = dram.get(0);
                    x = dram.get(0);
                    x = dram.get(1);

                    assert(dram.cache()->sum_hits() == 1);
                    assert(dram.cache()->sum_misses() == 2);
                    assert(dram.cache()->sum_flushes() == 0);
                    assert(dram.data()[0] == 0);
                    assert(dram.data()[1] == 1);
                    assert(dram.data()[2] == 2);
                    assert(dram.data()[3] == 3);
                }
                // Two vectors
                {
                    // A cache that can hold a single element
                    std::shared_ptr<LRUCache> cache
                        = std::make_shared<LRUCache>(sizeof(T), sizeof(T), 1);

                    VectorWithCache<T> dram0(cache);
                    dram0.data() = {0, 1, 2, 3};

                    VectorWithCache<T> dram1(cache);
                    dram1.data() = {4, 5, 6, 7};

                    int x;
                    x = dram0.get(0);
                    x = dram1.get(0);

                    assert(cache->sum_hits() == 0);
                    assert(cache->sum_misses() == 2);
                    assert(cache->sum_flushes() == 0);
                    assert(dram0.data()[0] == 0);
                    assert(dram0.data()[1] == 1);
                    assert(dram0.data()[2] == 2);
                    assert(dram0.data()[3] == 3);

                    assert(dram1.data()[0] == 4);
                    assert(dram1.data()[1] == 5);
                    assert(dram1.data()[2] == 6);
                    assert(dram1.data()[3] == 7);
                }

                return 0;
            }

        private:
            std::shared_ptr<Cache> _cache;
            std::vector<T> _data;
        };
    }
}
