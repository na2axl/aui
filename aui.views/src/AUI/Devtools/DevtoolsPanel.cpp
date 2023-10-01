// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

//#ifdef _DEBUG

#include "DevtoolsPanel.h"
#include "AUI/Common/AObject.h"
#include "AUI/Model/ITreeModel.h"
#include "AUI/Traits/values.h"
#include "AUI/View/ASplitter.h"
#include "AUI/View/AViewContainer.h"
#include "Devtools.h"
#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/ASS/ASS.h>
#include <AUI/View/AButton.h>


class ViewHierarchyTreeModel: public AObject, public ITreeModel<AString> {
private:
    _<AView> mRoot;

public:
    ViewHierarchyTreeModel(_<AView> root) : mRoot(std::move(root)) {
        scan(mRoot);
    }

    void scan(const _<AView>& view) {
        auto asContainer = _cast<AViewContainer>(view);
        if (asContainer) {
            return;
        }
        connect(asContainer->childrenChanged, this, [this, container = asContainer.get()]() {
            auto containerIndex = makeIndex(container);
            forEachDirectChildOf(containerIndex, [&](const ATreeModelIndex& i) {
                emit dataRemoved(i);
            });
            forEachDirectChildOf(containerIndex, [&](const ATreeModelIndex& i) {
                emit dataInserted(i);
            });
        });
    }

    static ATreeModelIndex makeIndex(aui::no_escape<AView> view) {
        std::size_t row = 0;
        if (auto p = view->getParent()) {
            row = p->getViews().indexOf(view->sharedPtr());
        }
        return ATreeModelIndex(row, 0, view->sharedPtr());
    }

    size_t childrenCount(const ATreeModelIndex& vertex) override {
        auto c = _cast<AViewContainer>(vertex.as<_<AView>>());
        if (c) {
            return c->getViews().size();
        }
        return 0;
    }

    AString itemAt(const ATreeModelIndex& index) override {
        return Devtools::prettyViewName(index.as<_<AView>>().get());
    }

    ATreeModelIndex indexOfChild(size_t row, size_t column, const ATreeModelIndex& vertex) override {
        auto c = _cast<AViewContainer>(vertex.as<_<AView>>());
        if (!c) {
            throw AException("invalid index");
        }
        return ATreeModelIndex(row, column, c->getViews().at(row));
    }

    ATreeModelIndex parent(const ATreeModelIndex& ofChild) override {
        auto view = ofChild.as<_<AView>>();
        auto parent = view->getParent();
        if (!parent) {
            return {};
        }

        auto parentOfParent = parent->getParent();

        return ATreeModelIndex{ parentOfParent ? parentOfParent->getViews().indexOf(parent->sharedPtr()) : 0, 0, parent->sharedPtr() };
    }

    ATreeModelIndex root() override {
        return ATreeModelIndex(0, 0, mRoot);
    }
};

DevtoolsPanel::DevtoolsPanel(ABaseWindow* targetWindow):
        mTargetWindow(targetWindow) {
    using namespace declarative;

    setContents(Vertical {
        Horizontal {
            Button { "Force layout update" }.clicked(me::forceLayoutUpdate),
            SpacerExpanding{},
            Label { "Use CTRL to hit test views" },
        },
        Horizontal::Expanding{
            mViewHierarchyTree = _new<ATreeView>() with_style{ass::MinSize{300_dp}, ass::Expanding{}},
            mViewPropertiesView = _new<ViewPropertiesView>(nullptr),
        },
    });
    auto model = _new<ViewHierarchyTreeModel>(aui::ptr::fake(targetWindow));
    mViewHierarchyTree->setModel(model);
    connect(mViewHierarchyTree->itemSelected, [this](const ATreeModelIndex& index) {
        mViewPropertiesView->setTargetView(index.as<_<AView>>());
    });
    connect(mViewHierarchyTree->itemMouseHover, [this](const ATreeModelIndex& index) {
        mViewPropertiesView->setTargetView(index.as<_<AView>>());
    });
    connect(mouseLeave, [this] {
        mTargetWindow->setProfiledView(nullptr);
        mTargetWindow->redraw();
    });
    connect(targetWindow->mouseMove, [this, targetWindow, model](glm::ivec2 position) {
        if (!AInput::isKeyDown(AInput::LCONTROL)) {
            return;
        }
        auto mouseOverView = targetWindow->getViewAtRecursive(position);
        if (!mouseOverView) {
            return;
        }

        mViewPropertiesView->setTargetView(mouseOverView);
        auto indexToSelect = model->find([&](const ATreeModelIndex& index) {
            return index.as<_<AView>>() == mouseOverView;
        });
        if (indexToSelect) {
            mViewHierarchyTree->select(*indexToSelect);
        }
    });
}

void DevtoolsPanel::forceLayoutUpdate() {
    mTargetWindow->updateLayout();
}
