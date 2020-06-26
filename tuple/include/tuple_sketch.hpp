/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef TUPLE_SKETCH_HPP_
#define TUPLE_SKETCH_HPP_

#include <string>

#include "serde.hpp"
#include "theta_update_sketch_base.hpp"

namespace datasketches {

// forward-declarations
template<typename S, typename SD, typename A> class tuple_sketch;
template<typename S, typename U, typename P, typename SD, typename A> class update_tuple_sketch;
template<typename S, typename SD, typename A> class compact_tuple_sketch;

template<
  typename Summary,
  typename SerDe = serde<Summary>,
  typename Allocator = std::allocator<Summary>
>
class tuple_sketch {
public:
  static const uint8_t SERIAL_VERSION = 3;

  virtual ~tuple_sketch() = default;

  /**
   * @return true if this sketch represents an empty set (not the same as no retained entries!)
   */
  virtual bool is_empty() const = 0;

  /**
   * @return estimate of the distinct count of the input stream
   */
  double get_estimate() const;

  /**
   * Returns the approximate lower error bound given a number of standard deviations.
   * This parameter is similar to the number of standard deviations of the normal distribution
   * and corresponds to approximately 67%, 95% and 99% confidence intervals.
   * @param num_std_devs number of Standard Deviations (1, 2 or 3)
   * @return the lower bound
   */
  double get_lower_bound(uint8_t num_std_devs) const;

  /**
   * Returns the approximate upper error bound given a number of standard deviations.
   * This parameter is similar to the number of standard deviations of the normal distribution
   * and corresponds to approximately 67%, 95% and 99% confidence intervals.
   * @param num_std_devs number of Standard Deviations (1, 2 or 3)
   * @return the upper bound
   */
  double get_upper_bound(uint8_t num_std_devs) const;

  /**
   * @return true if the sketch is in estimation mode (as opposed to exact mode)
   */
  bool is_estimation_mode() const;

  /**
   * @return theta as a fraction from 0 to 1 (effective sampling rate)
   */
  double get_theta() const;

  /**
   * @return theta as a positive integer between 0 and LLONG_MAX
   */
  virtual uint64_t get_theta64() const = 0;

  /**
   * @return the number of retained entries in the sketch
   */
  virtual uint32_t get_num_retained() const = 0;

  virtual uint16_t get_seed_hash() const = 0;

  /**
   * @return true if retained entries are ordered
   */
  virtual bool is_ordered() const = 0;

  /**
   * Writes a human-readable summary of this sketch to a given stream
   * @param print_items if true include the list of items retained by the sketch
   */
  virtual string<Allocator> to_string(bool print_items = false) const = 0;

  /**
   * This method serializes the sketch into a given stream in a binary form
   * @param os output stream
   */
//  virtual void serialize(std::ostream& os) const = 0;

  // This is a convenience alias for users
  // The type returned by the following serialize method
//  typedef vector_u8<Allocator> vector_bytes;

  /**
   * This method serializes the sketch as a vector of bytes.
   * An optional header can be reserved in front of the sketch.
   * It is an uninitialized space of a given size.
   * This header is used in Datasketches PostgreSQL extension.
   * @param header_size_bytes space to reserve in front of the sketch
   */
//  virtual vector_bytes serialize(unsigned header_size_bytes = 0) const = 0;

  // This is a convenience alias for users
  // The type returned by the following deserialize methods
  // It is not possible to return instances of an abstract type, so this has to be a pointer
//  typedef std::unique_ptr<tuple_sketch, std::function<void(tuple_sketch*)>> unique_ptr;

  /**
   * This method deserializes a sketch from a given stream.
   * @param is input stream
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of a sketch as a unique_ptr
   */
//  static unique_ptr deserialize(std::istream& is, uint64_t seed = DEFAULT_SEED);

  /**
   * This method deserializes a sketch from a given array of bytes.
   * @param bytes pointer to the array of bytes
   * @param size the size of the array
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of the sketch
   */
//  static unique_ptr deserialize(const void* bytes, size_t size, uint64_t seed = DEFAULT_SEED);

  using Entry = std::pair<uint64_t, Summary>;
  using const_iterator = theta_const_iterator<Entry, pair_extract_key<uint64_t, Summary>>;

  /**
   * Iterator over entries in this sketch.
   * @return begin iterator
   */
  virtual const_iterator begin() const = 0;

  /**
   * Iterator pointing past the valid range.
   * Not to be incremented or dereferenced.
   * @return end iterator
   */
  virtual const_iterator end() const = 0;

protected:
  enum flags { IS_BIG_ENDIAN, IS_READ_ONLY, IS_EMPTY, IS_COMPACT, IS_ORDERED };

  static uint16_t get_seed_hash(uint64_t seed);

  static void check_sketch_type(uint8_t actual, uint8_t expected);
  static void check_serial_version(uint8_t actual, uint8_t expected);
  static void check_seed_hash(uint16_t actual, uint16_t expected);
};

// update sketch

// for types with defined default constructor and + operation
template<typename Summary, typename Update>
struct default_update_policy {
  Summary create() const {
    return Summary();
  }
  void update(Summary& summary, const Update& update) const {
    summary += update;
  }
};

template<
  typename Summary,
  typename Update = Summary,
  typename Policy = default_update_policy<Summary, Update>,
  typename SerDe = serde<Summary>,
  typename Allocator = std::allocator<Summary>
>
class update_tuple_sketch: public tuple_sketch<Summary, SerDe, Allocator> {
public:
  using Base = tuple_sketch<Summary, SerDe, Allocator>;
  using Entry = typename Base::Entry;
  using AllocEntry = typename std::allocator_traits<Allocator>::template rebind_alloc<Entry>;
  using ExtractKey = pair_extract_key<uint64_t, Summary>;
  using tuple_map = theta_update_sketch_base<Entry, ExtractKey, AllocEntry>;
  using resize_factor = typename tuple_map::resize_factor;
  using const_iterator = typename Base::const_iterator;

  static const uint8_t SKETCH_TYPE = 2;

  // No constructor here. Use builder instead.
  class builder;

  virtual ~update_tuple_sketch() = default;

  virtual bool is_empty() const;
  virtual bool is_ordered() const;
  virtual uint64_t get_theta64() const;
  virtual uint32_t get_num_retained() const;
  virtual uint16_t get_seed_hash() const;
  virtual string<Allocator> to_string(bool print_items = false) const;

//  virtual void serialize(std::ostream& os) const;
//  typedef vector_u8<Allocator> vector_bytes; // alias for users
//  virtual vector_bytes serialize(unsigned header_size_bytes = 0) const;

  /**
   * @return configured nominal number of entries in the sketch
   */
  uint8_t get_lg_k() const;

  /**
   * @return configured resize factor of the sketch
   */
  resize_factor get_rf() const;

  /**
   * Update this sketch with a given string.
   * @param value string to update the sketch with
   */
  template<typename FwdUpdate>
  void update(const std::string& key, FwdUpdate&& value);

  /**
   * Update this sketch with a given unsigned 64-bit integer.
   * @param value uint64_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(uint64_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given signed 64-bit integer.
   * @param value int64_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(int64_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given unsigned 32-bit integer.
   * For compatibility with Java implementation.
   * @param value uint32_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(uint32_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given signed 32-bit integer.
   * For compatibility with Java implementation.
   * @param value int32_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(int32_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given unsigned 16-bit integer.
   * For compatibility with Java implementation.
   * @param value uint16_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(uint16_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given signed 16-bit integer.
   * For compatibility with Java implementation.
   * @param value int16_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(int16_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given unsigned 8-bit integer.
   * For compatibility with Java implementation.
   * @param value uint8_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(uint8_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given signed 8-bit integer.
   * For compatibility with Java implementation.
   * @param value int8_t to update the sketch with
   */
  template<typename FwdUpdate>
  void update(int8_t key, FwdUpdate&& value);

  /**
   * Update this sketch with a given double-precision floating point value.
   * For compatibility with Java implementation.
   * @param value double to update the sketch with
   */
  template<typename FwdUpdate>
  void update(double key, FwdUpdate&& value);

  /**
   * Update this sketch with a given floating point value.
   * For compatibility with Java implementation.
   * @param value float to update the sketch with
   */
  template<typename FwdUpdate>
  void update(float key, FwdUpdate&& value);

  /**
   * Update this sketch with given data of any type.
   * This is a "universal" update that covers all cases above,
   * but may produce different hashes.
   * Be very careful to hash input values consistently using the same approach
   * both over time and on different platforms
   * and while passing sketches between C++ environment and Java environment.
   * Otherwise two sketches that should represent overlapping sets will be disjoint
   * For instance, for signed 32-bit values call update(int32_t) method above,
   * which does widening conversion to int64_t, if compatibility with Java is expected
   * @param data pointer to the data
   * @param length of the data in bytes
   */
  template<typename FwdUpdate>
  void update(const void* key, size_t length, FwdUpdate&& value);

  /**
   * Remove retained entries in excess of the nominal size k (if any)
   */
  void trim();

  /**
   * Converts this sketch to a compact sketch (ordered or unordered).
   * @param ordered optional flag to specify if ordered sketch should be produced
   * @return compact sketch
   */
  compact_tuple_sketch<Summary, SerDe, Allocator> compact(bool ordered = true) const;

  virtual const_iterator begin() const;
  virtual const_iterator end() const;

  /**
   * This method deserializes a sketch from a given stream.
   * @param is input stream
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of a sketch
   */
//  static update_tuple_sketch deserialize(std::istream& is, uint64_t seed = DEFAULT_SEED);

  /**
   * This method deserializes a sketch from a given array of bytes.
   * @param bytes pointer to the array of bytes
   * @param size the size of the array
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of the sketch
   */
//  static update_tuple_sketch deserialize(const void* bytes, size_t size, uint64_t seed = DEFAULT_SEED);

  /**
   * Computes size needed to serialize the current state of the sketch.
   * This version is for fixed-size arithmetic types (integral and floating point).
   * @return size in bytes needed to serialize this sketch
   */
//  template<typename SS = Summary, typename std::enable_if<std::is_arithmetic<SS>::value, int>::type = 0>
//  size_t get_serialized_size_bytes() const;

  /**
   * Computes size needed to serialize the current state of the sketch.
   * This version is for all other types and can be expensive since every item needs to be looked at.
   * @return size in bytes needed to serialize this sketch
   */
//  template<typename SS = Summary, typename std::enable_if<!std::is_arithmetic<SS>::value, int>::type = 0>
//  size_t get_serialized_size_bytes() const;

private:
  Policy policy_;
  tuple_map map_;

  // for builder
  update_tuple_sketch(uint8_t lg_cur_size, uint8_t lg_nom_size, resize_factor rf, float p, uint64_t seed, const Policy& policy);
};

// compact sketch

template<
  typename Summary,
  typename SerDe = serde<Summary>,
  typename Allocator = std::allocator<Summary>
>
class compact_tuple_sketch: public tuple_sketch<Summary, SerDe, Allocator> {
public:
  using Base = tuple_sketch<Summary, SerDe, Allocator>;
  using Entry = typename Base::Entry;
  using AllocEntry = typename std::allocator_traits<Allocator>::template rebind_alloc<Entry>;
  using const_iterator = typename Base::const_iterator;
  using AllocBytes = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
  using vector_bytes = std::vector<uint8_t, AllocBytes>;
  using ExtractKey = pair_extract_key<uint64_t, Summary>;
  using comparator = compare_by_key<Entry, ExtractKey>;

  static const uint8_t SKETCH_TYPE = 3;

  // Instances of this type can be obtained:
  // - by compacting an update_tuple_sketch
  // - as a result of a set operation
  // - by deserializing a previously serialized compact sketch

  compact_tuple_sketch(const Base& other, bool ordered);
  virtual ~compact_tuple_sketch() = default;

  virtual bool is_empty() const;
  virtual bool is_ordered() const;
  virtual uint64_t get_theta64() const;
  virtual uint32_t get_num_retained() const;
  virtual uint16_t get_seed_hash() const;
  virtual string<Allocator> to_string(bool print_items = false) const;

  void serialize(std::ostream& os) const;
  vector_bytes serialize(unsigned header_size_bytes = 0) const;

  virtual const_iterator begin() const;
  virtual const_iterator end() const;

  /**
   * This method deserializes a sketch from a given stream.
   * @param is input stream
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of a sketch
   */
  static compact_tuple_sketch deserialize(std::istream& is, uint64_t seed = DEFAULT_SEED);

  /**
   * This method deserializes a sketch from a given array of bytes.
   * @param bytes pointer to the array of bytes
   * @param size the size of the array
   * @param seed the seed for the hash function that was used to create the sketch
   * @return an instance of the sketch
   */
  static compact_tuple_sketch deserialize(const void* bytes, size_t size, uint64_t seed = DEFAULT_SEED);

  // TODO: try to hide this
  template<typename InputIt>
  compact_tuple_sketch(bool is_empty, bool is_ordered, uint16_t seed_hash, uint64_t theta, InputIt first, InputIt last);

  compact_tuple_sketch(bool is_empty, bool is_ordered, uint16_t seed_hash, uint64_t theta, std::vector<Entry, AllocEntry>&& entries);

private:
  bool is_empty_;
  bool is_ordered_;
  uint16_t seed_hash_;
  uint64_t theta_;
  std::vector<Entry, AllocEntry> entries_;

  /**
   * Computes size needed to serialize summaries in the sketch.
   * This version is for fixed-size arithmetic types (integral and floating point).
   * @return size in bytes needed to serialize summaries in this sketch
   */
  template<typename SS = Summary, typename std::enable_if<std::is_arithmetic<SS>::value, int>::type = 0>
  size_t get_serialized_size_summaries_bytes() const;

  /**
   * Computes size needed to serialize summaries in the sketch.
   * This version is for all other types and can be expensive since every item needs to be looked at.
   * @return size in bytes needed to serialize summaries in this sketch
   */
  template<typename SS = Summary, typename std::enable_if<!std::is_arithmetic<SS>::value, int>::type = 0>
  size_t get_serialized_size_summaries_bytes() const;

//  static compact_tuple_sketch<Summary, SerDe, Allocator> internal_deserialize(std::istream& is, uint8_t preamble_longs, uint8_t flags_byte, uint16_t seed_hash);
//  static compact_tuple_sketch<Summary, SerDe, Allocator> internal_deserialize(const void* bytes, size_t size, uint8_t preamble_longs, uint8_t flags_byte, uint16_t seed_hash);
};

// builder

template<typename S, typename U, typename P, typename SD, typename A>
class update_tuple_sketch<S, U, P, SD, A>::builder: public theta_base_builder<builder> {
public:
  /**
   * Creates and instance of the builder with default parameters.
   */
  builder(const P& policy = P());

  /**
   * This is to create an instance of the sketch with predefined parameters.
   * @return an instance of the sketch
   */
  update_tuple_sketch<S, U, P, SD, A> build() const;

private:
  P policy_;
};

} /* namespace datasketches */

#include "tuple_sketch_impl.hpp"

#endif
