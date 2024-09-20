#include "hydrosheds/tile_cache.hpp"

namespace hydrosheds {

auto TileCache::add_tile_to_cache(const TileKey &key,
                                  std::vector<char> &&tile_data) -> void {
  // If the cache is full, remove the least recently used tile
  if (tile_map_.size() >= max_tiles_) {
    auto deprecated_key = access_order_.back();
    access_order_.pop_back();
    tile_map_.erase(deprecated_key);
  }
  // Add the new tile to the cache
  tile_map_[key] = std::move(tile_data);
  access_order_.push_front(key);
}

}  // namespace hydrosheds
