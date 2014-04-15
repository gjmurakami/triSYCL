/* \file

   This is a simple C++ sequential OpenCL SYCL implementation to
   experiment with the OpenCL CL provisional specification.

   Ronan.Keryell at AMD point com

   This file is distributed under the University of Illinois Open Source
   License. See LICENSE.TXT for details.
*/


#include <type_traits>
#include "boost/multi_array.hpp"

/// SYCL dwells in the cl::sycl namespace
namespace cl {
namespace sycl {


/** Describe the type of access by kernels.

   This values should be normalized to allow separate compilation with
   different implementations?
*/
namespace access {
  /* By using "enum mode" here instead of "enum struct mode", we have for
     example "write" appearing both as cl::sycl::access::mode::write and
     cl::sycl::access::write, instead of only the last one. This seems
     more conform to the specification. */
  enum mode {
    read = 42, //< Why not? Insist on the fact that read_write != read + write
    write,
    atomic,
    read_write,
    discard_read_write
  };

  enum target {
    global_buffer = 2014, //< Just pick a random number...
    constant_buffer,
    local,
    image,
    host_buffer,
    host_image,
    image_array,
    cl_buffer,
    cl_image
  };

}


/// Define a multi-dimensional index range
template <size_t Dimensions = 1U>
struct range : std::vector<size_t> {
  /* Inherit the constructors from the parent

     Using a std::vector is overkill but std::array has no default
     constructors and I am lazzy to reimplement them
  */
  using std::vector<size_t>::vector;

  /// Create a 1D range from a single integer
  range(size_t S) : std::vector<size_t> { S } {}
};


/** SYCL queue, similar to the OpenCL queue concept.

    The implementation is quite minimal for now. :-)
*/
struct queue {};


/** The accessor abstracts the way buffer data are accessed inside a
    kernel in a multidimensional variable length array way.

    This implementation rely on boost::multi_array to provides this nice
    syntax and behaviour.

    Right now the aim of this class is just to access to the buffer in a
    read-write mode, even if capturing the multi_array_ref from a lambda
    make it const (since in some example we have lambda with [=] and
    without mutable). The access::mode is not used yet.
*/
template <access::mode M,
          typename ArrayType>
struct accessor {
  typedef typename std::remove_const<ArrayType>::type WritableArrayType;
  ArrayType Array;

  accessor(ArrayType &Buffer) :
    Array(static_cast<ArrayType>(Buffer)) {}

  /// This when we access to accessor[] that we override the const if any
  auto &operator[](size_t Index) const {
    return (const_cast<WritableArrayType &>(Array))[Index];
  }
};


/** A SYCL buffer is a multidimensional variable length array (à la C99
    VLA or even Fortran before) that is used to store data to work on.

    In the case we initialize it from a pointer, for now we just wrap the
    data with boost::multi_array_ref to provide the VLA semantics without
    any storage.
*/
template <typename T,
          size_t Dimensions = 1U>
struct buffer {
  using Implementation = boost::multi_array_ref<T, Dimensions>;
  // Extension to SYCL: provide pieces of STL container interface
  using element = T;
  using value_type = T;

  // If some allocation is requested, it is managed by this multi_array
  boost::multi_array<T, Dimensions> Allocation;
  // This is the multi-dimensional interface to the data
  boost::multi_array_ref<T, Dimensions> Access;
  // If the data are read-only, store the information for later optimization
  bool ReadOnly ;


  /// Create a new buffer of size \param r
  buffer(range<Dimensions> r) : Allocation(r),
                                Access(Allocation),
                                ReadOnly(false) {}


  /** Create a new buffer from \param host_data of size \param r without
      further allocation */
  buffer(T * host_data, range<Dimensions> r) : Access(host_data, r),
                                               ReadOnly(false) {}


  /** Create a new read only buffer from \param host_data of size \param r
      without further allocation */
  buffer(const T * host_data, range<Dimensions> r) :
    Access(host_data, r),
    ReadOnly(true) {}


  /// \todo
  //buffer(storage<T> &store, range<dimensions> r)

  /// Create a new allocated 1D buffer from the given elements
  buffer(T * start_iterator, T * end_iterator) :
    // Construct Access for nothing since there is no default constructor
    Access(Allocation) {
    /* Then reinitialize Allocation since this is the only multi_array
       method with this iterator interface */
    Allocation.assign(start_iterator, end_iterator);
    // And rewrap it in the multi_array_ref
    Access = Allocation;
  }


  /// Create a new buffer from an old one, with a new allocation
  buffer(buffer<T, Dimensions> &b) : Allocation(b.Access),
                                     Access(Allocation),
                                     ReadOnly(false) {}


  /** Create a new sub-buffer without allocation to have separate accessors
      later */
  /* \todo
  buffer(buffer<T, dimensions> b,
         index<dimensions> base_index,
         range<dimensions> sub_range)
  */

  // Allow CLHPP objects too?
  // \todo
  /*
  buffer(cl_mem mem_object,
         queue from_queue,
         event available_event)
  */

  // Use BOOST_DISABLE_ASSERTS at some time to disable range checking

  /// Return an accessor of the required mode \param M
  template <access::mode M>
  accessor<M, Implementation> get_access() {
    return { Access };
  }

};



/** SYCL command group gather all the commands needed to execute one or
    more kernels in a kind of atomic way. Since all the parameters are
    captured at command group creation, one can execute the content in an
    asynchronous way and delayed schedule.

    For now just execute the command group directly.
 */
struct command_group {
  template <typename Functor>
  command_group(queue Q, Functor F) {
    F();
  }
};


template <typename KernalName, typename Functor>
Functor kernel_lambda(Functor F) {
  return F;
}


/** SYCL single task lauches a computation without parallelism at launch
    time.

    Right now the implementation does nothing else that forwarding the
    execution of the given functor
*/
auto single_task = [] (auto F) { F(); };

}
}

/*
    # Some Emacs stuff:
    ### Local Variables:
    ### minor-mode: flyspell
    ### ispell-local-dictionary: "american"
    ### End:
*/