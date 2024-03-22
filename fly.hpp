#pragma once
#include <stdexcept>
#include <string>

template <typename T>
struct FlyResource {
  FlyResource() {}
  FlyResource(const FlyResource& x) { *this = x; }
  FlyResource(FlyResource&& x) { *this = std::move(x); }
  FlyResource& operator=(const FlyResource& x) {
    hash = x.hash;
    i = x.i;
    return *this;
  }
  FlyResource& operator=(FlyResource&& x) {
    std::swap(hash, x.hash);
    std::swap(i, x.i);
    return *this;
  }
  FlyResource(const T& val) {
    hash = std::hash<T>()(val);
    auto& t = fly[hash];
    const auto found = std::find(t.begin(), t.end(), val);
    if (found == t.end()) {
      i = t.size();
      t.push_back(val);
    }
    else {
      i = found - t.begin();
    }
  }

  const T& val() const { if (i == std::numeric_limits<size_t>::max()) throw std::runtime_error("Uninitialised FlyResource accessed"); return fly[hash][i]; }
  explicit operator const T&() const { return val(); }
  const T* const operator->() const { return &val(); }
  const T& operator*() const { return val(); }

  bool operator==(const FlyResource& x) const {
    return hash == x.hash && i == x.i;
  }

  bool operator==(const T& x) const {
    FlyResource xf(x);
    return *this == xf;
  }

private:
  using fly_t = std::unordered_map<size_t, std::vector<const T>>;
  static inline fly_t fly;
  size_t hash {};
  size_t i { std::numeric_limits<size_t>::max() };

  friend std::hash<FlyResource>;
};

template <typename T>
struct std::hash<FlyResource<T>> {
  constexpr size_t operator()(const FlyResource<T>& k) const {
    return k.hash;
  }
};

using FlyString = FlyResource<std::string>;
