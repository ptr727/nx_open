// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include <QtCore/Qt>

namespace nx::utils::algorithm {

/**
 * Merge sorted lists using priority queue concept implemented via STL heap.
 *
 * @param sortedLists List of source sorted lists. Its type should be random access iterable.
 *   Each list should be sequentially iterable and have value_type type
 *   and size, push_back and erase(first, last) methods.
 * @param lessItem Item comparison functor taking two values of sorted list's value_type
 *   and returning a boolean. Must be consistent with source lists sort order.
 * @param totalLimit The maximum allowed number of items in the resulting list; ignored if <= 0.
 * @returns Single merged sorted list.
 */
template<class ListOfSortedLists, class LessPredicate>
auto merge_sorted_lists(
    ListOfSortedLists sortedLists,
    LessPredicate lessItem,
    int totalLimit = std::numeric_limits<int>::max(),
    bool removeDuplicates = true)
{
    using SortedList = std::remove_reference_t<decltype(*sortedLists.begin())>;
    using DestinationList = std::remove_cv_t<SortedList>;

    if (totalLimit <= 0)
        totalLimit = std::numeric_limits<int>::max(); //< No limit.

    const auto processOneList =
        [removeDuplicates, lessItem](DestinationList list, int limit)
        {
            if (removeDuplicates)
            {
                const auto equals =
                    [lessItem](const auto& l, const auto& r)
                    {
                        return !(lessItem(l, r) || lessItem(r, l));
                    };

                list.erase(
                    std::unique(list.begin(), list.end(), equals),
                    list.end());
            }

            if ((int) list.size() <= limit)
                return list;

            // Truncate.
            auto outOfLimit = list.begin();
            std::advance(outOfLimit, limit);
            list.erase(outOfLimit, list.end());

            return list;
        };

    switch (sortedLists.size())
    {
        case 0: return SortedList();
        case 1: return processOneList(std::move(*sortedLists.begin()), totalLimit);
        default: break;
    }

    using Iterator = decltype(sortedLists.begin()->begin());
    using IteratorRange = std::pair<Iterator, Iterator>;

    std::vector<IteratorRange> queueData;
    queueData.reserve(sortedLists.size());

    SortedList* lastNonEmptyList = nullptr;
    for (auto& list: sortedLists)
    {
        if (list.empty())
            continue;

        lastNonEmptyList = &list;
        queueData.push_back(std::make_pair(list.begin(), list.end()));
    }

    switch (queueData.size())
    {
        case 0: return SortedList();
        case 1: return processOneList(std::move(*lastNonEmptyList), totalLimit);
        default: break;
    }

    DestinationList result;
    static constexpr int kMaximumReserve = 10000; // Something sensible.

    result.reserve(std::min(totalLimit, kMaximumReserve));

    const auto lessPriority =
        [lessItem](const IteratorRange& left, const IteratorRange& right) -> bool
        {
            return lessItem(*right.first, *left.first);
        };

    std::make_heap(queueData.begin(), queueData.end(), lessPriority);

    while (!queueData.empty() && ((int) result.size()) < totalLimit)
    {
        std::pop_heap(queueData.begin(), queueData.end(), lessPriority);
        auto& range = queueData.back();

        if (!removeDuplicates || result.empty() || lessItem(result.back(), *range.first))
            result.push_back(std::move(*range.first));

        ++range.first;

        if (range.first == range.second)
            queueData.pop_back();
        else
            std::push_heap(queueData.begin(), queueData.end(), lessPriority);
    }

    return result;
};

/**
 * Merge sorted lists using priority queue concept implemented via STL heap.
 *
 * @param sortedLists List of source sorted lists. Its type should be random access iterable.
 *   Each list should be sequentially iterable and have value_type type
 *   and size, push_back and erase(first, last) methods.
 * @param sortFieldGetter A functor taking value_type of a sorted list and returning a value
 *   of type that can be compared with operator <.
 * @param sortOrder How source sorted lists are sorted.
 * @param totalLimit The maximum allowed number of items in the resulting list; ignored if <= 0.
 * @returns Single merged sorted list.
 */
template<class ListOfSortedLists, class SortFieldGetter>
auto merge_sorted_lists(
    ListOfSortedLists sortedLists,
    SortFieldGetter sortFieldGetter,
    Qt::SortOrder sortOrder = Qt::AscendingOrder,
    int totalLimit = std::numeric_limits<int>::max(),
    bool removeDuplicates = true)
{
    using SortedList = std::remove_reference_t<decltype(*sortedLists.begin())>;
    using Item = typename SortedList::value_type;

    using Less = std::function<bool(const Item&, const Item&)>;
    const auto less = sortOrder == Qt::AscendingOrder
        ? Less([key = sortFieldGetter](const Item& l, const Item& r) { return key(l) < key(r); })
        : Less([key = sortFieldGetter](const Item& l, const Item& r) { return key(l) > key(r); });

    return merge_sorted_lists(std::move(sortedLists), less, totalLimit, removeDuplicates);
};

/**
 * Merge sorted lists using priority queue concept implemented via STL heap.
 *
 * @param sortedLists List of source sorted lists. Its type should be random access iterable.
 *   Each list should be sequentially iterable and have value_type type
 *   and size, push_back and erase(first, last) methods.
 *   Values should be comparable with operator<
 * @param sortOrder How source sorted lists are sorted.
 * @param totalLimit The maximum allowed number of items in the resulting list; ignored if <= 0.
 * @returns Single merged sorted list.
 */
template<class ListOfSortedLists>
auto merge_sorted_lists(
    ListOfSortedLists sortedLists,
    Qt::SortOrder sortOrder = Qt::AscendingOrder,
    int totalLimit = std::numeric_limits<int>::max(),
    bool removeDuplicates = true)
{
    using SortedList = std::remove_reference_t<decltype(*sortedLists.begin())>;

    return merge_sorted_lists(std::move(sortedLists),
        [](const typename SortedList::value_type& value) { return value; },
        sortOrder, totalLimit, removeDuplicates);
}

} // namespace nx::utils::algorithm
