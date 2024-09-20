#pragma once

#include <cstddef>
#include <list>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace hydrosheds {

/// @brief Represents a key for a tile in the cache.
using TileKey = std::tuple<int, int>;

}  // namespace hydrosheds

namespace std {

/// @brief Hash specialization for the TileKey type.
template <>
struct hash<hydrosheds::TileKey> {
  /// @brief Hashes a TileKey object.
  /// @param[in] key The TileKey object to hash.
  /// @return The hash value of the TileKey object.
  std::size_t operator()(const hydrosheds::TileKey &key) const {
    return std::hash<int>()(std::get<0>(key)) ^
           std::hash<int>()(std::get<1>(key));
  }
};

}  // namespace std

namespace hydrosheds {

/// @brief A simple tile cache implementation.
class TileCache {
 public:
  /// @brief Constructs a TileCache object with a given maximum number of tiles.
  /// @param[in] max_tiles The maximum number of tiles that the cache can hold.
  explicit TileCache(size_t max_tiles) : max_tiles_(max_tiles) {}

  /// @brief Checks if a tile is in the cache.
  /// @param[in] key The key of the tile to check.
  /// @return true if the tile is in the cache, false otherwise.
  inline auto is_tile_in_cache(const TileKey &key) const noexcept -> bool {
    return tile_map_.find(key) != tile_map_.end();
  }

  /// @brief Adds a tile to the cache.
  /// @param[in] key The key of the tile to add.
  /// @param[in] tile_data The data of the tile to add.
  auto add_tile_to_cache(const TileKey &key,
                         std::vector<char> &&tile_data) -> void;

  /// @brief Gets a tile from the cache.
  /// @param[in] key The key of the tile to get.
  /// @return A reference to the tile data.
  inline auto get_tile_from_cache(const TileKey &key) -> std::vector<char> & {
    access_order_.remove(key);
    access_order_.push_front(key);
    return tile_map_[key];
  }

 private:
  /// @brief Maximum number of tiles that the cache can hold.
  size_t max_tiles_;
  /// @brief Map of tiles in the cache.
  std::unordered_map<TileKey, std::vector<char>> tile_map_{};
  /// @brief List of tiles in the cache in access order.
  std::list<TileKey> access_order_{};
};

}  // namespace hydrosheds
