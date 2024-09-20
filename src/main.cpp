#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hydrosheds/dataset.hpp"

PYBIND11_MODULE(hydrosheds, m) {
  pybind11::class_<hydrosheds::Dataset>(m, "Dataset")
      .def(pybind11::init<const std::vector<std::string> &, int, size_t,
                          size_t>(),
           pybind11::arg("paths"), pybind11::arg("espg_code") = 4326,
           pybind11::arg("tile_size") = 256,
           pybind11::arg("max_cache_size") = 4096)
      .def(
          "is_water",
          [](hydrosheds::Dataset &hs, hydrosheds::ConstRefVectorFloat64 lon,
             hydrosheds::ConstRefVectorFloat64 lat,
             size_t num_threads) { return hs.is_water(lon, lat, num_threads); },
          pybind11::arg("lon"), pybind11::arg("lat"),
          pybind11::arg("num_threads") = 0,
          pybind11::call_guard<pybind11::gil_scoped_release>());
}
