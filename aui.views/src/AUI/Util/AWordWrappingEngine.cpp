//
// Created by Alex2772 on 11/16/2021.
//

#include <AUI/Traits/iterators.h>
#include "AWordWrappingEngine.h"
#include <algorithm>
#include <numeric>
#include <AUI/Util/AFraction.h>

void AWordWrappingEngine::performLayout(const glm::ivec2& offset, const glm::ivec2& size) {
    if (mEntries.empty()) return;


    struct StandardEntry {
        _<Entry> entry;
        int occupiedHorizontalSpace;
    };

    struct FloatingEntry {
        _<Entry> entry;
        int occupiedHorizontalSpace;
        int remainingHeight;
    };


    AVector<AVector<StandardEntry>> inflatedEntriesByRows;
    decltype(inflatedEntriesByRows)::iterator currentRow;
    size_t currentRowWidth = 0;
    int currentRowHeight = 0;
    int currentY = offset.y;

    AVector<FloatingEntry> leftFloat;
    AVector<FloatingEntry> rightFloat;

    auto beginRow = [&] {
        currentRowWidth = 0;
        for (auto& i : leftFloat) {
            currentRowWidth += i.occupiedHorizontalSpace;
        }
        for (auto& i : rightFloat) {
            currentRowWidth += i.occupiedHorizontalSpace;
        }
        inflatedEntriesByRows.push_back({});
        currentRow = inflatedEntriesByRows.end() - 1;
    };

    auto flushRow = [&](bool last) {
        int currentPos = 0;
        switch (mTextAlign) {
            case TextAlign::JUSTIFY: {
                if (!last) {
                    int actualRowWidth = 0;
                    int leftPadding = 0;
                    int rightPadding = 0;

                    for (auto& i: leftFloat) leftPadding += i.occupiedHorizontalSpace;
                    for (auto& i: rightFloat) rightPadding += i.occupiedHorizontalSpace;
                    for (auto& i: *currentRow) actualRowWidth += i.occupiedHorizontalSpace;

                    int freeSpace = size.x - leftPadding - rightPadding;

                    AFraction spacing(freeSpace - actualRowWidth, (glm::max)(int(currentRow->size()) - 1, 1));

                    int currentPos = offset.x + leftPadding;
                    int index = 0;
                    for (auto& i: *currentRow) {
                        i.entry->setPosition({currentPos + (spacing * index).toInt(), currentY});
                        currentPos += i.occupiedHorizontalSpace;
                        ++index;
                    }
                    break;
                }
                // fallthrough
            }
            case TextAlign::LEFT:
                for (auto& i: leftFloat) {
                    currentPos += i.occupiedHorizontalSpace;
                }
                for (auto& i: *currentRow) {
                    i.entry->setPosition({currentPos + offset.x, currentY});
                    currentPos += i.occupiedHorizontalSpace;
                }
                break;

            case TextAlign::CENTER: {
                int actualRowWidth = 0;
                int leftPadding = 0;
                int rightPadding = 0;

                for (auto& i: leftFloat) leftPadding += i.occupiedHorizontalSpace;
                for (auto& i: rightFloat) rightPadding += i.occupiedHorizontalSpace;
                for (auto& i: *currentRow) actualRowWidth += i.occupiedHorizontalSpace;

                currentPos = leftPadding + (size.x - leftPadding - rightPadding - actualRowWidth) / 2;
                for (auto& i: *currentRow) {
                    i.entry->setPosition({currentPos + offset.x, currentY});
                    currentPos += i.occupiedHorizontalSpace;
                }
                break;
            }

            case TextAlign::RIGHT:
                // calculate actual row width
                int actualRowWidth = 0;
                for (auto& i : *currentRow) actualRowWidth += i.occupiedHorizontalSpace;
                for (auto& i : rightFloat) actualRowWidth += i.occupiedHorizontalSpace;
                currentPos = size.x - actualRowWidth;
                for (auto& i : *currentRow) {
                    i.entry->setPosition({currentPos + offset.x, currentY});
                    currentPos += i.occupiedHorizontalSpace;
                }
                break;
        }
    };


    beginRow();

    for (auto currentItem = mEntries.begin(); currentItem != mEntries.end(); ++currentItem) {
        auto currentItemSize = (*currentItem)->getSize();

        // check if entry fits into the row
        if (currentRowWidth + currentItemSize.x  > size.x) {
            // if current row is empty, we must place this element in any case
            if (!currentRow->empty()) {
                // jump to the next row

                auto removeRedundantItems = [&currentRowHeight](AVector<FloatingEntry>& fl) {
                    for (auto it = fl.begin(); it != fl.end();) {
                        it->remainingHeight -= currentRowHeight;
                        if (it->remainingHeight <= 0) {
                            it = fl.erase(it);
                        } else {
                            ++it;
                        }
                    }
                };

                flushRow(false);

                removeRedundantItems(leftFloat);
                removeRedundantItems(rightFloat);

                currentY += currentRowHeight;
                currentRowHeight = 0;
                beginRow();
            }
        }
        switch ((*currentItem)->getFloat()) {
            case Float::LEFT: {
                leftFloat.push_back({*currentItem, currentItemSize.x, currentItemSize.y});
                int position = 0;
                for (auto it = leftFloat.begin(); it != leftFloat.end() - 1; ++it) {
                    position += it->occupiedHorizontalSpace;
                }
                (*currentItem)->setPosition({position, currentY});
                break;
            }

            case Float::RIGHT: {
                rightFloat.push_back({*currentItem, currentItemSize.x, currentItemSize.y});
                int position = size.x;
                for (auto it = rightFloat.begin(); it != rightFloat.end(); ++it) {
                    position -= it->occupiedHorizontalSpace;
                }
                (*currentItem)->setPosition({position, currentY});
                break;
            }

            case Float::NONE:
                currentRow->push_back({*currentItem, currentItemSize.x});
                currentRowHeight = glm::max(currentRowHeight, currentItemSize.y);
                break;
        }

        currentRowWidth += currentItemSize.x;
    }
    if (!currentRow->empty()) {
        flushRow(true);
    }
}