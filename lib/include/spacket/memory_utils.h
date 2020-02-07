#pragma once

#include <memory>


template <typename T>
using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

template <typename T>
struct NopDeleter {
    void operator()(T* ptr) { if (ptr != nullptr) { ptr->~T(); } }
};

template <typename T>
using StaticPtr = std::unique_ptr<T, NopDeleter<T>>;

template <typename T, typename...Args>
StaticPtr<T> makeStatic(void* storage, Args&&...args) {
    return StaticPtr<T>(new (storage) T(std::forward<Args>(args)...));
}