#pragma once

#include <array>
#include <cstddef>

namespace hydrosheds {

/// @brief Represents a bounding box defined by geographical coordinates.
///
/// The BBox class provides a way to define a rectangular bounding box using
/// geographical coordinates and check if a given point lies within this box.
///
class BBox {
 public:
  /// @brief Constructs a BBox object using geotransform parameters and
  /// dimensions.
  ///
  /// @param[in] geotransform An array of 6 doubles representing the
  /// geotransform parameters.
  /// @param[in] x_size The size of the bounding box in the x-direction.
  /// @param[in] y_size The size of the bounding box in the y-direction.
  constexpr BBox(const std::array<double, 6> &geotransform, const size_t x_size,
                 size_t y_size) noexcept
      : min_x_(geotransform[0]),
        max_x_(geotransform[0] + geotransform[1] * x_size),
        min_y_(geotransform[3] + geotransform[5] * y_size),
        max_y_(geotransform[3]) {}

  /// @brief Checks if a given point (longitude, latitude) is within the
  /// bounding box.
  ///
  /// @param[in] lon The longitude of the point.
  /// @param[in] lat The latitude of the point.
  /// @return true if the point is within the bounding box, false otherwise.
  ////
  constexpr bool contains(double lon, double lat) const noexcept {
    return lon >= min_x_ && lon <= max_x_ && lat >= min_y_ && lat <= max_y_;
  }

  /// @brief Gets the minimum x-coordinate of the bounding box.
  ///
  /// @return The minimum x-coordinate of the bounding box.
  constexpr auto min_x() const noexcept -> double { return min_x_; }

  /// @brief Gets the maximum x-coordinate of the bounding box.
  ///
  /// @return The maximum x-coordinate of the bounding box.
  constexpr auto max_x() const noexcept -> double { return max_x_; }

  /// @brief Gets the minimum y-coordinate of the bounding box.
  ///
  /// @return The minimum y-coordinate of the bounding box.
  constexpr auto min_y() const noexcept -> double { return min_y_; }

  /// @brief Gets the maximum y-coordinate of the bounding box.
  ///
  /// @return The maximum y-coordinate of the bounding box.
  constexpr auto max_y() const noexcept -> double { return max_y_; }

 private:
  /// @brief The minimum x-coordinate of the bounding box.
  double min_x_;
  /// @brief The maximum x-coordinate of the bounding box.
  double max_x_;
  /// @brief The minimum y-coordinate of the bounding box.
  double min_y_;
  /// @brief The maximum y-coordinate of the bounding box.
  double max_y_;
};

}  // namespace hydrosheds
