//
//  main.cpp
//  test_poly
//
//  following watching this video https://youtu.be/OtU51Ytfe04..
//
//  The concept of type erasure is very powerful and very useful.
//  In the toy example below we show how we can use a constrained 
//  (through sfinae) type erasure container to enable a form of
//  static polymorphism, that is safe, does not allocate memory
//  on the heap, and is extremely fast because it entirely 
//  avoids the usage of the typical virtual table lookups inherent 
//  to polymorphic behaviors.
//
//  There remain some significant barriors to use though.  Primarily
//  making a base class for these types of type erasure containers 
//  means that someone must make both a interface class, and a
//  type erasure wrapper.  The type erasure wrapper can be confusing
//  to make if people aren't familiar with some more esoteric 
//  c++ language features. 
//
//  The general idea in the code below is to embed references to the 
//  functions in the real instance into the std::function which is 
//  filled with a lambda upon construction.  std::function can allocate
//  if the function is large enough, but these functions don't need 
//  to capture or to be large since they are just a dispatch method
//  for the interface in the type erasure instance to call the instance
//  of the real object. 
//


#include <string>
#include <iostream>
#include <vector>
#include <type_traits>

#define MAX_BUFFER_SIZE 64

struct VehicleInterface {
  virtual void accelerate(int x) const = 0;
};


/**
 * Each function implemented in your interface class can be duplicated into this
 * type erasure container as part of it's public interface, where its implementation
 * does nothing except dispatch the to the real function which is accessed through
 * capturing the original type in a lambda, storing the real object into the a local
 * buffer via 'placement new', and then dispatching it to that local object through
 * the lambda with a cast.
 */
class Vehicle {
  std::aligned_storage_t<MAX_BUFFER_SIZE> buffer_; // cache for the object this wraps

  std::function<void(int)>accelerate_impl_;

  void * buffer_ptr_; // allows us to put placement new in init list but costs
                      // an extra ptr in size.

  void (*buffer_deleter_)(void*) noexcept ;

public:
  template <typename Any,
    typename = std::enable_if_t<std::is_base_of<VehicleInterface, Any>::value>
  >
  Vehicle(Any && vehicle)
    : accelerate_impl_([this](int x) noexcept {
          using T = decltype(vehicle);
          return reinterpret_cast<T>(buffer_).accelerate(x);
        }
      ),
      buffer_ptr_(new (&buffer_) Any(std::forward<Any>(vehicle))),
      buffer_deleter_([](void * ptr) noexcept {
          using T = std::decay_t<decltype(vehicle)>;
          reinterpret_cast<T*>(&ptr)->~T();
        }
      )
  {
    // save only objects that are less than 64 bytes
    static_assert(
      sizeof(Any) <= MAX_BUFFER_SIZE,
      "you type must not exceed 64 bytes"
    );
  }
  
  ~Vehicle() noexcept {
    buffer_deleter_(reinterpret_cast<void*>(&buffer_));
  }
  
  // public interface that presents the same interface as the
  // base interface but dispatches the call to the real class.
  auto accelerate(int x) const {
    return accelerate_impl_(x);
  }
};


// some example classes which inherit from the interfaces
struct Car2 : VehicleInterface {
  void accelerate(int x) const {
    std::cout << "car increasing speed by " + std::to_string(x) + "\n";
  }
};
struct AirPlane2 : VehicleInterface {
  void accelerate(int x) const {
    std::cout << "plane increasing speed by " + std::to_string(x * 2) + "\n";
  }
};

// drive it
int main(int argc, const char * argv[]) {
  std::cout << "size of Vehicle container : " << sizeof(Vehicle) << '\n';
  
  std::vector<Vehicle> vehicles2 = {Car2{}, AirPlane2{}};
  for (auto const & v : vehicles2) {
    v.accelerate(3);
  }
  return 0;
}
