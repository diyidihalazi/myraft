#ifndef MYUTIL_MAKE_UNIQUE_H_
#define MYUTIL_MAKE_UNIQUE_H_

#include <memory>

namespace myutil {

template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts... params) {
  return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

} // namespace myutil

#endif // MYUTIL_MAKE_UNIQUE_H_
