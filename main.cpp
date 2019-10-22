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
//  Created by Christopher Goebel on 10/5/19.
//  Copyright © 2019 Christopher Goebel. All rights reserved.
//


#include <string>
#include <iostream>
#include <vector>

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
  std::string type_tag;
  std::function<void(int)>accelerate_impl;
  
  std::aligned_storage<64> buffer_;
  
public:
  template <typename Any,
    typename = std::enable_if_t<std::is_base_of<VehicleInterface, Any>::value>
  >
  Vehicle(Any && vehicle)
    : type_tag(std::string(typeid(Any).name())),
      accelerate_impl([this, &vehicle](int x){
        using T = decltype(vehicle);
        return reinterpret_cast<T>(buffer_).accelerate(x);
        }
      )
  {
    static_assert(sizeof(Any) <= MAX_BUFFER_SIZE, "you type must not exceed 64 bytes");
    new (&buffer_) Any(std::forward<Any>(vehicle));
  }
  
  
  auto accelerate(int x) const {
    return accelerate_impl(x);
  }
};


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

int main(int argc, const char * argv[]) {
  std::vector<Vehicle> vehicles2 = {Car2{}, AirPlane2{}};
  for (auto const & v : vehicles2) {
    v.accelerate(3);
  }
  return 0;
}