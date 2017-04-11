#pragma once

#include "spin-mutex.h"
#include "sol.hpp"
#include <mutex>
#include <map>
#include <set>

namespace effil {

// Unique handle for all objects spawned from one object.
typedef void* GCObjectHandle;

static const GCObjectHandle GCNull = nullptr;

// All effil objects that owned in lua code have to inherit this class.
// This type o object can persist in multiple threads and in multiple lua states.
// Child has to care about storing data and concurrent access.
class GCObject {
public:
    GCObject() noexcept
            : refs_(new std::set<GCObjectHandle>) {}
    GCObject(const GCObject& init) = default;
    GCObject(GCObject&& init) = default;
    virtual ~GCObject() = default;

    GCObjectHandle handle() const noexcept { return reinterpret_cast<GCObjectHandle>(refs_.get()); }
    size_t instances() const noexcept { return refs_.use_count(); }
    const std::set<GCObjectHandle>& refers() const { return *refs_; }

protected:
    std::shared_ptr<std::set<GCObjectHandle>> refs_;
};

enum class GCState { Idle, Running, Stopped };

class GarbageCollector {
public:
    GarbageCollector();
    ~GarbageCollector() = default;

    // This method is used to create all managed objects.
    template <typename ObjectType, typename... Args>
    ObjectType create(Args&&... args) {
        if (lastCleanup_.fetch_add(1) == step_)
            cleanup();
        auto object = std::make_shared<ObjectType>(std::forward<Args>(args)...);

        std::lock_guard<std::mutex> g(lock_);
        objects_[object->handle()] = object;
        return *object;
    }

    GCObject* get(GCObjectHandle handle);
    bool has(GCObjectHandle handle) const;
    void cleanup();
    size_t size() const;
    void stop();
    void resume();
    size_t step() const { return step_; }
    void step(size_t newStep) { step_ = newStep; }
    GCState state() const { return state_; }

private:
    mutable std::mutex lock_;
    GCState state_;
    std::atomic<size_t> lastCleanup_;
    size_t step_;
    std::map<GCObjectHandle, std::shared_ptr<GCObject>> objects_;

private:
    GarbageCollector(GarbageCollector&&) = delete;
    GarbageCollector(const GarbageCollector&) = delete;
};

GarbageCollector& getGC();

} // effil
