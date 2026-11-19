#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace us {

// speed of sound in tissue (1540 m/s)
constexpr float SPEED_OF_SOUND = 1540.0f;

// 40MHz ADC sampling rate
constexpr float SAMPLING_FREQ = 40.0e6f;

// 5 MHz ultrasound wave frequency
constexpr float CENTER_FREQ = 5.0e6f;

// number of transducer microphones
constexpr size_t NUM_CHANNELS = 128;

// 0.3 mm distance between microphones
constexpr float ELEMENT_PITCH = 0.3e-3f;

// time samples for each microphone channel
constexpr size_t NUM_SAMPLES = 2048;

// ultrasound probe width
constexpr float PROBE_WIDTH = (NUM_CHANNELS - 1) * ELEMENT_PITCH;

// maps 2d image pixel coordinates to physical tissue coordinates in meters
struct GridConfig {
  size_t num_x = 256;                  // columns
  size_t num_z = 256;                  // rows
  float x_start = -PROBE_WIDTH / 2.0f; // left edge of probe
  float x_end = PROBE_WIDTH / 2.0f;    // right edge of probe
  float z_start = 5.0e-3f;             // start depth (5mm)
  float z_end = 45.0e-3f;              // end depth (45mm)

  // horizontal distance per pixel
  [[nodiscard]] constexpr float dx() const noexcept {
    return num_x > 1 ? (x_end - x_start) / static_cast<float>(num_x - 1) : 0.0f;
  }

  // vertical distance per pixel
  [[nodiscard]] constexpr float dz() const noexcept {
    return num_z > 1 ? (z_end - z_start) / static_cast<float>(num_z - 1) : 0.0f;
  }

  // physical x coordinate of column col in meters
  [[nodiscard]] constexpr float x_at(size_t col) const noexcept {
    return x_start + static_cast<float>(col) * dx();
  }

  // physical z coordinate of row row in meters
  [[nodiscard]] constexpr float z_at(size_t row) const noexcept {
    return z_start + static_cast<float>(row) * dz();
  }
};

// 64-byte aligned flat 2D memory buffer matching CPU cache line width
template <typename T> class AlignedBuffer {
private:
  size_t rows_ = 0;
  size_t cols_ = 0;
  T *data_ = nullptr;

public:
  // default constructor
  AlignedBuffer() noexcept : rows_(0), cols_(0), data_(nullptr) {}

  // main constructor
  AlignedBuffer(size_t rows, size_t cols)
      : rows_(rows), cols_(cols), data_(nullptr) {

    if (rows_ > 0 && cols_ > 0) {
      size_t bytes = rows_ * cols_ * sizeof(T);
      // round up bytes to nearest multiple of 64
      size_t alloc_bytes = (bytes + 63) & ~size_t(63);

      data_ = static_cast<T *>(aligned_alloc(64, alloc_bytes));

      if (!data_) {
        throw std::bad_alloc();
      }
      std::fill_n(data_, rows_ * cols_, T{});
    }
  }

  // destructor
  ~AlignedBuffer() {
    if (data_) {
      std::free(data_);
    }
  }
  // disable copy constructor
  AlignedBuffer(const AlignedBuffer &) = delete;

  // disable copy assigment constructor
  AlignedBuffer &operator=(const AlignedBuffer &) = delete;

  // move constructor
  AlignedBuffer(AlignedBuffer &&other) noexcept
      : rows_(other.rows_), cols_(other.cols_), data_(other.data_) {
    other.rows_ = 0;
    other.cols_ = 0;
    other.data_ = nullptr;
  }

  // move assignment operator
  AlignedBuffer &operator=(AlignedBuffer &&other) noexcept {
    if (this != &other) {
      if (data_) {
        std::free(data_);
      }
      rows_ = other.rows_;
      cols_ = other.cols_;
      data_ = other.data_;
      other.rows_ = 0;
      other.cols_ = 0;
      other.data_ = nullptr;
    }
    return *this;
  }

  // element access at (row, col)
  [[nodiscard]] T &at(size_t row, size_t col) noexcept {
    return data_[row * cols_ + col];
  }

  // const element access
  [[nodiscard]] const T &at(size_t row, size_t col) const noexcept {
    return data_[row * cols_ + col];
  }

  // raw pointer to start of row
  [[nodiscard]] T *row_ptr(size_t row) noexcept {
    return data_ + (row * cols_);
  }

  // const row pointer to start of row
  [[nodiscard]] const T *row_ptr(size_t row) const noexcept {
    return data_ + (row * cols_);
  }

  // pointer to the flat data
  [[nodiscard]] T *data() noexcept { return data_; }

  // const pointer to the flat data
  [[nodiscard]] const T *data() const noexcept { return data_; }

  // number of rows
  [[nodiscard]] size_t rows() const noexcept { return rows_; }

  // number of columns
  [[nodiscard]] size_t cols() const noexcept { return cols_; }

  // total number of elements
  [[nodiscard]] size_t size() const noexcept { return rows_ * cols_; }

  // Check if buffer is empty
  [[nodiscard]] bool empty() const noexcept { return data_ == nullptr; }
};

using RFData = AlignedBuffer<float>;
using BModeImage = AlignedBuffer<float>;

} // namespace us