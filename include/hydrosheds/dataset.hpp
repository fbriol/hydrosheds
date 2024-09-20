#pragma once

#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <Eigen/Core>
#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "hydrosheds/bbox.hpp"
#include "hydrosheds/tile_cache.hpp"

namespace hydrosheds {

/// @brief Alias for a vector of boolean values.
using VectorBool = Eigen::Array<bool, Eigen::Dynamic, 1>;

/// @brief Alias for a vector of double values.
using VectorFloat64 = Eigen::Array<double, Eigen::Dynamic, 1>;

/// @brief Alias for a constant reference to a vector of double values.
using ConstRefVectorFloat64 = const Eigen::Ref<const VectorFloat64> &;

/// @brief Holds a pointer to a GDALDataset object and a custom deleter.
using GDALDatasetSmartPtr =
    std::unique_ptr<GDALDataset, void (*)(GDALDataset *)>;

/// @brief Holds a pointer to an OGRCoordinateTransformation object and a custom
/// deleter.
using OGRCoordinateTransformationSmartPtr =
    std::unique_ptr<OGRCoordinateTransformation,
                    void (*)(OGRCoordinateTransformation *)>;

/// @brief Represents a HydroSHEDS dataset and provides a method to check if a
/// given point is water.
class Dataset {
 public:
  /// @brief Constructs a Dataset object with a list of paths to HydroSHEDS
  /// datasets, a tile size, and a maximum cache size.
  ///
  /// @param[in] paths A list of paths to HydroSHEDS datasets.
  /// @param[in] espg_code The EPSG code used to transform the input coordinates
  /// to the dataset's projection. Defaults to 4326.
  /// @param[in] tile_size The size of the tiles used to cache the datasets.
  /// Defaults to 256.
  /// @param[in] max_cache_size The maximum number of tiles that the cache can
  /// hold. Defaults to 4096.
  Dataset(const std::vector<std::string> &paths, int espg_code = 4326,
          size_t tile_size = 256, size_t max_cache_size = 4096)
      : tile_size_(tile_size),
        max_cache_size_(max_cache_size),
        espg_code_(espg_code) {
    GDALAllRegister();

    for (const auto &path : paths) {
      base_datasets_.emplace_back(init_dataset_info(path));
    }
  }

  /// @brief Checks if a given point is water.
  ///
  /// This function checks if a given point is water by checking if it is
  /// within the bounding box of any of the datasets. If the point is within
  /// the bounding box of a dataset, the function checks if the point is water
  /// by checking if the value of the dataset at the point is less than 0.
  ///
  /// @param[in] lon The longitude of the points.
  /// @param[in] lat The latitude of the points.
  /// @param[in] num_threads The number of threads to use for parallelization.
  auto is_water(ConstRefVectorFloat64 lon, ConstRefVectorFloat64 lat,
                size_t num_threads = 0) const -> VectorBool;

 private:
  /// @brief Represents information about a HydroSHEDS dataset.
  struct DatasetInfo {
    /// @brief GDAL dataset pointer.
    GDALDatasetSmartPtr dataset;
    /// @brief Coordinate transformation pointer.
    OGRCoordinateTransformationSmartPtr transform;
    /// @brief Geotransform parameters.
    std::array<double, 6> geotransform;
    /// @brief Mutex to protect the dataset from concurrent access.
    std::unique_ptr<std::mutex> mutex;
    /// @brief Bounding box of the dataset.
    BBox bbox;
    /// @brief Size of the dataset in the x-direction.
    size_t x_size;
    /// @brief Size of the dataset in the y-direction.
    size_t y_size;

    /// @brief Constructs a DatasetInfo object with a GDAL dataset pointer, a
    /// coordinate transformation pointer, geotransform parameters, a mutex, a
    /// bounding box, and the size of the dataset in the x and y directions.
    ///
    /// @param[in] dataset GDAL dataset pointer.
    /// @param[in] transform Coordinate transformation pointer.
    /// @param[in] geotransform Geotransform parameters.
    /// @param[in] mutex Mutex to protect the dataset from concurrent access.
    /// @param[in] bbox Bounding box of the dataset.
    /// @param[in] x_size Size of the dataset in the x-direction.
    /// @param[in] y_size Size of the dataset in the y-direction.
    DatasetInfo(GDALDatasetSmartPtr dataset,
                OGRCoordinateTransformationSmartPtr transform,
                std::array<double, 6> geotransform,
                std::unique_ptr<std::mutex> mutex, BBox bbox, size_t x_size,
                size_t y_size)
        : dataset(std::move(dataset)),
          transform(std::move(transform)),
          geotransform(geotransform),
          mutex(std::move(mutex)),
          bbox(std::move(bbox)),
          x_size(x_size),
          y_size(y_size) {}
  };

  /// @brief Represents a cache for a HydroSHEDS dataset.
  struct DatsetCache {
    /// @brief Pointer to the dataset information.
    DatasetInfo *dataset_info;
    /// @brief Tile cache for the dataset.
    TileCache tile_cache;

    /// @brief Constructs a DatsetCache object with a pointer to the dataset
    /// information and a tile cache.
    ///
    /// @param[in] dataset_info Pointer to the dataset information.
    /// @param[in] tile_cache Tile cache for the dataset.
    DatsetCache(DatasetInfo *dataset_info, TileCache tile_cache)
        : dataset_info(dataset_info), tile_cache(tile_cache) {}
  };

  /// @brief List of base datasets handled by the object.
  std::vector<std::unique_ptr<DatasetInfo>> base_datasets_;

  /// @brief Size of the tiles used to cache the datasets.
  size_t tile_size_;

  /// @brief Maximum number of tiles that the cache can hold.
  size_t max_cache_size_;

  /// @brief ESPG code used to transform the input coordinates to the dataset's
  /// projection.
  int espg_code_;

  /// @brief Determines the properties of a HydroSHEDS dataset.
  /// @param[in] path The path to the HydroSHEDS dataset.
  /// @return A pointer to a DatasetInfo object.
  auto init_dataset_info(const std::string &path)
      -> std::unique_ptr<DatasetInfo>;

  /// @brief Allocates a cache for the datasets.
  /// @param[in] num_threads The number of threads to use for parallelization.
  /// @return A vector of DatasetCache objects.
  auto allocate_cache(size_t num_threads) const -> std::vector<DatsetCache>;

  /// @brief Loads a tile from the cache.
  /// @param[in] tile_key The key of the tile to load.
  /// @param[in,out] dataset_cache The cache to load the tile from.
  auto load_tile_cache(const TileKey &tile_key,
                       DatsetCache &dataset_cache) const -> void;

  /// @brief Determines if a point is water.
  /// @param[in] lon Longitude of the point.
  /// @param[in] lat Latitude of the point.
  /// @param[in,out] dataset_cache_collection Collection of dataset caches.
  /// @return True if the point is water, false otherwise.
  auto is_water(double lon, double lat,
                std::vector<DatsetCache> &dataset_cache_collection) const
      -> bool;

  /// @brief Determines if a point is water.
  /// @param[in] lon Longitude of the point.
  /// @param[in] lat Latitude of the point.
  /// @param[in,out] dataset_cache Collection of dataset caches.
  /// @return True if the point is water, false otherwise.
  auto is_water(double lon, double lat,
                DatsetCache &dataset_cache) const -> bool;
};

}  // namespace hydrosheds
