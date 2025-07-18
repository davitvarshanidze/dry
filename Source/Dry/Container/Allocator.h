//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2023 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#ifdef DRY_IS_BUILDING
#include "Dry.h"
#else
#include <Dry/Dry.h>
#endif

#include <cstddef>

namespace Dry
{

struct AllocatorBlock;
struct AllocatorNode;

/// %Allocator memory block.
struct AllocatorBlock
{
    /// Size of a node.
    unsigned nodeSize_;
    /// Number of nodes in this block.
    unsigned capacity_;
    /// First free node.
    AllocatorNode* free_;
    /// Next allocator block.
    AllocatorBlock* next_;
    /// Nodes follow.
};

/// %Allocator node.
struct AllocatorNode
{
    /// Next free node.
    AllocatorNode* next_;
    /// Data follows.
};

/// Initialize a fixed-size allocator with the node size and initial capacity.
DRY_API AllocatorBlock* AllocatorInitialize(unsigned nodeSize, unsigned initialCapacity = 1);
/// Uninitialize a fixed-size allocator. Frees all blocks in the chain.
DRY_API void AllocatorUninitialize(AllocatorBlock* allocator);
/// Reserve a node. Creates a new block if necessary.
DRY_API void* AllocatorReserve(AllocatorBlock* allocator);
/// Free a node. Does not free any blocks.
DRY_API void AllocatorFree(AllocatorBlock* allocator, void* ptr);

/// %Allocator template class. Allocates objects of a specific class.
template <class T> class Allocator
{
public:
    /// Construct.
    explicit Allocator(unsigned initialCapacity = 0) :
        allocator_(nullptr)
    {
        if (initialCapacity)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T), initialCapacity);
    }

    /// Destruct.
    ~Allocator()
    {
        AllocatorUninitialize(allocator_);
    }

    /// Prevent copy construction.
    Allocator(const Allocator<T>& rhs) = delete;
    /// Prevent assignment.
    Allocator<T>& operator =(const Allocator<T>& rhs) = delete;

    /// Reserve and default-construct an object.
    T* Reserve()
    {
        if (!allocator_)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T));
        auto* newObject = static_cast<T*>(AllocatorReserve(allocator_));
        new(newObject) T();

        return newObject;
    }

    /// Reserve and copy-construct an object.
    T* Reserve(const T& object)
    {
        if (!allocator_)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T));
        auto* newObject = static_cast<T*>(AllocatorReserve(allocator_));
        new(newObject) T(object);

        return newObject;
    }

    /// Destruct and free an object.
    void Free(T* object)
    {
        (object)->~T();
        AllocatorFree(allocator_, object);
    }

private:
    /// Allocator block.
    AllocatorBlock* allocator_;
};

}
