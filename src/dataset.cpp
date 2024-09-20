#include "hydrosheds/dataset.hpp"

#include "hydrosheds/parallel_for.hpp"

namespace hydrosheds {

// Create a coordinate transformation from the dataset's projection to lat/lon
inline auto create_coordinate_transformation(const GDALDatasetSmartPtr &dataset,
                                             const int espg_code)
    -> OGRCoordinateTransformationSmartPtr {
  OGRSpatialReference srs;
  const char *wkt = dataset->GetProjectionRef();
  srs.importFromWkt(&wkt);
  OGRSpatialReference srs_latlon;
  if (srs_latlon.importFromEPSG(espg_code) != OGRERR_NONE) {
    throw std::runtime_error("Invalid EPSG code: " + std::to_string(espg_code));
  }
  return OGRCoordinateTransformationSmartPtr(
      OGRCreateCoordinateTransformation(&srs_latlon, &srs),
      [](OGRCoordinateTransformation *ct) {
        OCTDestroyCoordinateTransformation(ct);
      });
}

auto Dataset::init_dataset_info(const std::string &path)
    -> std::unique_ptr<DatasetInfo> {
  auto dataset = GDALDatasetSmartPtr(
      reinterpret_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly)),
      [](GDALDataset *ds) { GDALClose(ds); });
  if (!dataset) {
    throw std::runtime_error("Failed to open GeoTIFF file: " + path);
  }

  std::array<double, 6> geotransform;
  if (dataset->GetGeoTransform(geotransform.data()) != CE_None) {
    throw std::runtime_error("Failed to get geotransform for file: " + path);
  }

  size_t x_size = static_cast<size_t>(dataset->GetRasterXSize());
  size_t y_size = static_cast<size_t>(dataset->GetRasterYSize());

  BBox bbox(geotransform, x_size, y_size);

  auto transform = create_coordinate_transformation(dataset, espg_code_);
  if (!transform) {
    throw std::runtime_error(
        "Failed to create coordinate transformation for file: " + path);
  }

  return std::make_unique<DatasetInfo>(
      std::move(dataset), std::move(transform), std::move(geotransform),
      std::make_unique<std::mutex>(), std::move(bbox), x_size, y_size);
}

// auto Dataset::display_dataset_info(
//     std::function<void(const std::string &)> display) const -> void {
//   for (const auto &dataset : base_datasets_) {
//     display("Dataset: ");
//     display("  Bounding box: ");
//     display("    min_x: " + std::to_string(dataset->bbox.min_x()));
//     display("    max_x: " + std::to_string(dataset->bbox.max_x()));
//     display("    min_y: " + std::to_string(dataset->bbox.min_y()));
//     display("    max_y: " + std::to_string(dataset->bbox.max_y()));
//     display("  Size: ");
//     display("    x_size: " + std::to_string(dataset->x_size));
//     display("    y_size: " + std::to_string(dataset->y_size));
//     display("  Projection: ");
//     display("    " + std::string(dataset->dataset->GetProjectionRef()));
//   }
// }

auto Dataset::allocate_cache(size_t num_threads) const
    -> std::vector<DatsetCache> {
  std::vector<DatsetCache> cache;
  cache.reserve(num_threads);
  for (auto &dataset : base_datasets_) {
    cache.emplace_back(dataset.get(), TileCache(max_cache_size_));
  }
  return cache;
}

auto Dataset::is_water(ConstRefVectorFloat64 lon, ConstRefVectorFloat64 lat,
                       size_t num_threads) const -> VectorBool {
  if (lon.size() != lat.size()) {
    throw std::invalid_argument("lon and lat must have the same size");
  }
  auto result = VectorBool(lon.size());
  result.setZero();

  auto worker = [&](size_t start, size_t end) {
    auto cache = allocate_cache(num_threads);
    for (size_t ix = start; ix < end; ix++) {
      result(ix) = is_water(lon(ix), lat(ix), cache);
    }
  };
  parallel_for(worker, lon.size(), num_threads);
  return result;
}

auto Dataset::is_water(double lon, double lat,
                       std::vector<DatsetCache> &dataset_cache_collection) const
    -> bool {
  for (auto &item : dataset_cache_collection) {
    if (item.dataset_info->bbox.contains(lon, lat)) {
      if (is_water(lon, lat, item)) {
        return true;
      }
    }
  }
  return false;
}

auto Dataset::is_water(double lon, double lat,
                       DatsetCache &dataset_cache) const -> bool {
  double x = lon;
  double y = lat;
  auto *dataset_info = dataset_cache.dataset_info;
  if (!dataset_info->transform->Transform(1, &x, &y)) {
    throw std::runtime_error("Failed to transform coordinates.");
  }

  const auto &geotransform = dataset_info->geotransform;
  auto pixel_x = static_cast<size_t>((x - geotransform[0]) / geotransform[1]);
  auto pixel_y = static_cast<size_t>((y - geotransform[3]) / geotransform[5]);

  // Calculate the tile indices
  auto tile_x = pixel_x / tile_size_;
  auto tile_y = pixel_y / tile_size_;
  auto tile_key = TileKey(tile_x, tile_y);

  // Check if the tile is in the cache
  if (!dataset_cache.tile_cache.is_tile_in_cache(tile_key)) {
    load_tile_cache(tile_key, dataset_cache);
  }

  // Get the tile data
  auto &tile_data = dataset_cache.tile_cache.get_tile_from_cache(tile_key);

  // Calculate the pixel's position within the tile
  auto local_x = pixel_x % tile_size_;
  auto local_y = pixel_y % tile_size_;

  // Get the value in the tile
  return tile_data[local_y * tile_size_ + local_x] == 1;
}

auto Dataset::load_tile_cache(const TileKey &tile_key,
                              DatsetCache &dataset_cache) const -> void {
  auto &dataset_info = *dataset_cache.dataset_info;
  auto &tile_cache = dataset_cache.tile_cache;

  // Load a tile from the dataset into the cache
  auto x_offset = std::get<0>(tile_key) * tile_size_;
  auto y_offset = std::get<1>(tile_key) * tile_size_;
  auto x_size = std::min(tile_size_, dataset_info.x_size - x_offset);
  auto y_size = std::min(tile_size_, dataset_info.y_size - y_offset);

  auto tile_data = std::vector<char>(tile_size_ * tile_size_);

  //   printf("x_offset: %d\n", x_offset);
  //   printf("y_offset: %d\n", y_offset);
  //   printf("x_size: %d\n", x_size);
  //   printf("y_size: %d\n", y_size);
  //   printf("dataset_info.x_size: %d\n", dataset_info.x_size);
  //   printf("dataset_info.y_size: %d\n", dataset_info.y_size);
  //   fflush(stdout);

  if (x_offset >= dataset_info.x_size || y_offset >= dataset_info.y_size ||
      x_offset + x_size > dataset_info.x_size ||
      y_offset + y_size > dataset_info.y_size || x_offset < 0 || y_offset < 0) {
    throw std::runtime_error("Requested tile is out of bounds.");
  }

  // Read the tile from the dataset. Lock the mutex to prevent concurrent
  // access to the dataset.
  {
    std::lock_guard<std::mutex> lock(*dataset_info.mutex);
    auto band = dataset_info.dataset->GetRasterBand(1);
    if (band->RasterIO(GF_Read, x_offset, y_offset, x_size, y_size,
                       tile_data.data(), tile_size_, tile_size_, GDT_Byte, 0,
                       0) != CE_None) {
      throw std::runtime_error("Failed to read tile from dataset.");
    }
  }
  tile_cache.add_tile_to_cache(tile_key, std::move(tile_data));
}

}  // namespace hydrosheds
