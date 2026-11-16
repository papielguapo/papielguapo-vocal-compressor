#include <new>
#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace
{
thread_local bool monitoring = false;
thread_local unsigned long allocations = 0;
void record() noexcept { if (monitoring) ++allocations; }
}

void beginAllocationMonitoring() noexcept { allocations = 0; monitoring = true; }
unsigned long endAllocationMonitoring() noexcept { monitoring = false; return allocations; }

void* operator new(std::size_t size)
{
    record();
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

void* operator new(std::size_t size, std::align_val_t alignment)
{
    record();
    const auto a = static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    if (auto* p = _aligned_malloc(size == 0 ? 1 : size, a)) return p;
#else
    const auto rounded = ((size == 0 ? 1 : size) + a - 1) / a * a;
    if (auto* p = std::aligned_alloc(a, rounded)) return p;
#endif
    throw std::bad_alloc();
}
void* operator new[](std::size_t size, std::align_val_t alignment) { return ::operator new(size, alignment); }
void operator delete(void* p, std::align_val_t) noexcept
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t a) noexcept { ::operator delete(p, a); }
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }
void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }
