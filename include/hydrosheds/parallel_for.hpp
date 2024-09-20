#pragma once

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

namespace hydrosheds {

/// @brief Parallelizes a for loop using a given number of threads.
///
/// This function parallelizes a for loop using a given number of threads.
/// The function takes a lambda function as input, which is called for each
/// iteration of the loop. The lambda function should take two size_t
/// arguments, which represent the start and end indices of the loop.
///
/// @tparam Lambda The type of the lambda function to be called for each
/// iteration of the loop.
/// @param[in] worker The lambda function to be called for each iteration of the
/// loop.
/// @param[in] size The size of the loop.
/// @param[in] num_threads The number of threads to use for parallelization. If
/// set to 0, the function will use the number of hardware threads available.
template <typename Lambda>
void parallel_for(const Lambda &worker, size_t size, size_t num_threads) {
  if (num_threads == 1) {
    worker(0, size);
    return;
  }

  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  // Adjust num_threads to not exceed the size
  num_threads = std::min(num_threads, size);

  // List of threads responsible for parallelizing the calculation
  std::vector<std::thread> threads;
  std::exception_ptr exception = nullptr;

  // Access index to the vectors required for calculation
  size_t start = 0;
  size_t shift = size / num_threads;

  threads.reserve(num_threads);

  // Launch threads
  for (size_t ix = 0; ix < num_threads; ++ix) {
    size_t end = (ix == num_threads - 1) ? size : start + shift;
    threads.emplace_back([&worker, start, end, &exception]() {
      try {
        worker(start, end);
      } catch (...) {
        exception = std::current_exception();
      }
    });
    start += shift;
  }

  // Join threads
  for (auto &&thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // Rethrow the last exception caught
  if (exception) {
    std::rethrow_exception(exception);
  }
}

}  // namespace hydrosheds
