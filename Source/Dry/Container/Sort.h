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

#include "../Container/Swap.h"
#include "../Container/VectorBase.h"

namespace Dry
{

static const int QUICKSORT_THRESHOLD = 16;

// Based on Comparison of several sorting algorithms by Juha Nieminen
// http://warp.povusers.org/SortComparison/

/// Perform insertion sort on an array.
template <class T> void InsertionSort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter)
{
    for (RandomAccessIterator<T> i = beginIter + 1; i < endIter; ++i)
    {
        T temp = *i;
        RandomAccessIterator<T> j = i;
        while (j > beginIter && temp < *(j - 1))
        {
            *j = *(j - 1);
            --j;
        }
        *j = temp;
    }
}

/// Perform insertion sort on an array using a compare function.
template <class T, class U> void InsertionSort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter, U compare)
{
    for (RandomAccessIterator<T> i = beginIter + 1; i < endIter; ++i)
    {
        T temp = *i;
        RandomAccessIterator<T> j = i;
        while (j > beginIter && compare(temp, *(j - 1)))
        {
            *j = *(j - 1);
            --j;
        }
        *j = temp;
    }
}

/// Perform quick sort initial pass on an array. Does not sort fully.
template <class T> void InitialQuickSort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter)
{
    while (endIter - beginIter > QUICKSORT_THRESHOLD)
    {
        // Choose the pivot by median
        RandomAccessIterator<T> pivot = beginIter + ((endIter - beginIter) / 2);
        if (*beginIter < *pivot && *(endIter - 1) < *beginIter)
            pivot = beginIter;
        else if (*(endIter - 1) < *pivot && *beginIter < *(endIter - 1))
            pivot = endIter - 1;

        // Partition and sort recursively
        RandomAccessIterator<T> i = beginIter - 1;
        RandomAccessIterator<T> j = endIter;
        T pivotValue = *pivot;
        for (;;)
        {
            while (pivotValue < *(--j));
            while (*(++i) < pivotValue);
            if (i < j)
                Swap(*i, *j);
            else
                break;
        }

        InitialQuickSort(beginIter, j + 1);
        beginIter = j + 1;
    }
}

/// Perform quick sort initial pass on an array using a compare function. Does not sort fully.
template <class T, class U> void InitialQuickSort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter, U compare)
{
    while (endIter - beginIter > QUICKSORT_THRESHOLD)
    {
        // Choose the pivot by median
        RandomAccessIterator<T> pivot = beginIter + ((endIter - beginIter) / 2);
        if (compare(*beginIter, *pivot) && compare(*(endIter - 1), *beginIter))
            pivot = beginIter;
        else if (compare(*(endIter - 1), *pivot) && compare(*beginIter, *(endIter - 1)))
            pivot = endIter - 1;

        // Partition and sort recursively
        RandomAccessIterator<T> i = beginIter - 1;
        RandomAccessIterator<T> j = endIter;
        T pivotValue = *pivot;
        for (;;)
        {
            while (compare(pivotValue, *(--j)));
            while (compare(*(++i), pivotValue));
            if (i < j)
                Swap(*i, *j);
            else
                break;
        }

        InitialQuickSort(beginIter, j + 1, compare);
        beginIter = j + 1;
    }
}

/// Sort in ascending order using quicksort for initial passes, then an insertion sort to finalize.
template <class T> void Sort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter)
{
    InitialQuickSort(beginIter, endIter);
    InsertionSort(beginIter, endIter);
}

/// Sort in ascending order using quicksort for initial passes, then an insertion sort to finalize, using a compare function.
template <class T, class U> void Sort(RandomAccessIterator<T> beginIter, RandomAccessIterator<T> endIter, U compare)
{
    InitialQuickSort(beginIter, endIter, compare);
    InsertionSort(beginIter, endIter, compare);
}

}
