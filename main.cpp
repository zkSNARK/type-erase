//
//  main.cpp
//  test_poly
//
//  following watching this video https://youtu.be/OtU51Ytfe04..
//
//  The concept of type erasure is very powerful and very useful.
//  There remain some significant barriors to use though.
//  virtual table.  Just embed references to the actual functions
//  directly in the class.  Using std::function and a lambda, we
//  can capture the original object via reference, and stash the
//  object into a local buffer
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
  std::string type_tag_; // currently unused but could use for unerasure?
  std::function<void(int)>accelerate_impl_;
  
  std::aligned_storage<64> buffer_; // cache for the object this wraps
  std::function<void(void)> buffer_deleter_; // store deleter for buffer's placement new
  
public:
  template <typename Any,
    typename = std::enable_if_t<std::is_base_of<VehicleInterface, Any>::value>
  >
  Vehicle(Any && vehicle)
    : type_tag_(std::string(typeid(Any).name())),
      accelerate_impl_([this, &vehicle](int x) {
          using T = decltype(vehicle);
          return reinterpret_cast<T>(buffer_).accelerate(x);
        }
      ),
      buffer_deleter_([this, &vehicle] {
          using T = std::remove_cv_t<std::remove_reference_t<decltype(vehicle)>>;
          reinterpret_cast<T*>(&buffer_)->~T();
        }
      )
  {
    // save only objects that are less than 64 bytes
    static_assert(
      sizeof(Any) <= MAX_BUFFER_SIZE,
      "you type must not exceed 64 bytes"
    );
    
    // placement new
    new (&buffer_) Any(std::forward<Any>(vehicle));
  }
  
  ~Vehicle() {
    buffer_deleter_();
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
  std::vector<Vehicle> vehicles2 = {Car2{}, AirPlane2{}};
  for (auto const & v : vehicles2) {
    v.accelerate(3);
  }
  return 0;
}
