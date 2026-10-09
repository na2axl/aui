/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2025 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <AUI/Model/IListModel.h>
#include <AUI/Model/IMutableListModel.h>
#include <AUI/Common/AVector.h>
#include <functional>

template <typename T>
class AListModelFilter : public IListModel<T> {
public:
    /**
     * @brief Type-erased predicate.
     * @details
     * Every lambda-expression has its own closure type. See [setFilter].
     */
    using filter_type = std::function<bool(const T&)>;

    using value_type = T;

    explicit AListModelFilter(const _<IListModel<T>>& other, filter_type&& adapter) :
        mOther(other),
        mFilter(std::forward<filter_type>(adapter)) {
        fill();

        AObject::connect(other->dataChanged, this, [&](const AListModelRange<T>&) { invalidate(); });
        AObject::connect(other->dataInserted, this, [&](const AListModelRange<T>&) { invalidate(); });
        AObject::connect(other->dataRemoved, this, [&](const AListModelRange<T>&) { invalidate(); });
    }


    ~AListModelFilter() override = default;

    size_t listSize() override {
        return mMapping.size();
    }

    T listItemAt(const AListModelIndex& index) override {
        return mOther->listItemAt(mMapping[index.getRow()]);
    }

    /**
     * Removes all items and performs filtering again for all elements.
     */
    void invalidate() {
        size_t prevSize = mMapping.size();
        mMapping.clear();
        fill();

        size_t currentSize = mMapping.size();
        if (currentSize == prevSize) {
            emit this->dataChanged(this->range(0, currentSize));
        } else if (currentSize > prevSize) {
            emit this->dataChanged(this->range(0, prevSize));
            emit this->dataInserted(this->range(prevSize, currentSize));
        } else {
            emit this->dataChanged(this->range(0, currentSize));
            emit this->dataRemoved(this->range(currentSize, prevSize));
        }
    }

    /**
     * @brief Replaces the predicate and re-applies it.
     * @param filter new predicate; the filter runs immediately, so this may notify observers.
     */
    void setFilter(filter_type filter) {
        mFilter = std::move(filter);
        invalidate();
    }

    /**
     * @brief Performs filtering for currently unfiltered elements.
     * @details
     * If you guarantee, that filter's previous results is a subset of filter's new results, you should use this
     * function instead of [invalidate] because it's faster.
     */
    void lazyInvalidate() {
        auto ranges = this->rangesIncluding([&](size_t i) {
            return !mFilter(listItemAt(i));
        });
        int offset = 0;
        for (const AListModelRange<T>& r : ranges) {
            mMapping.erase(mMapping.begin() + r.getBegin().getRow() - offset,
                           mMapping.begin() + r.getEnd().getRow()) - offset;
            offset += r.getEnd().getRow() - r.getBegin().getRow();
        }
    }

private:
    _<IListModel<T>> mOther;
    filter_type mFilter;
    AVector<size_t> mMapping;

    void fill() {
        for (size_t i = 0; i < mOther->listSize(); ++i) {
            if (mFilter(mOther->listItemAt(i))) {
                mMapping.push_back(i);
            }
        }
    }
};

namespace AModels {
template <typename Container>
auto filter(const _<Container>& other, typename AListModelFilter<typename Container::value_type>::filter_type&& filter)
    -> _<AListModelFilter<typename Container::value_type>> {
    return aui::ptr::manage_shared(new AListModelFilter<typename Container::value_type>(
        other, std::forward<typename AListModelFilter<typename Container::value_type>::filter_type>(filter)));
}
}