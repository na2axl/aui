/*
 * AUI Framework - Declarative UI toolkit for modern C++20
 * Copyright (C) 2020-2024 Alex2772 and Contributors
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AUI/Traits/concepts.h"
#include <type_traits>
#include <AUI/Common/SharedPtr.h>
#include <AUI/Common/ASignal.h>
#include <AUI/Traits/members.h>


template<typename View, typename FieldType>
struct ADataBindingDefault {
public:
    /**
     * Called then view linked with field.
     * @param view
     */
    static void setup(const _<View>& view) {}

    static ASignal<FieldType>(View::*getGetter()) {
        return nullptr;
    }
    static void(View::*getSetter())(const FieldType& v) {
        return nullptr;
    }
};

template <typename Model>
class ADataBinding;

template<typename Model, typename Klass, typename ModelField, typename Getter, typename Setter>
class ADataBindingLinker {
private:
    ADataBinding<Model>* mBinder;
    void(Klass::*mSetterFunc)(Setter);
    std::decay_t<ModelField>(Model::*mField);
    ASignal<std::decay_t<Getter>>(Klass::*mGetter);

public:
    ADataBindingLinker(ADataBinding<Model>* binder, ASignal<std::decay_t<Getter>>(Klass::*getter), void (Klass::*setterFunc)(Setter),
                       std::decay_t<ModelField>(Model::*field)):
            mBinder(binder), mGetter(getter), mSetterFunc(setterFunc), mField(field) {}

    ADataBinding<Model>* getBinder() const {
        return mBinder;
    }

    auto getSetterFunc() const {
        return mSetterFunc;
    }

    auto getField() const {
        return mField;
    }

    auto getGetter() const {
        return mGetter;
    }
};
template<typename Model, typename Data>
class ADataBindingLinker2 {
private:
    ADataBinding<Model>* mBinder;
    Data(Model::*mField);

public:
    ADataBindingLinker2(ADataBinding<Model>* binder, Data(Model::*field)) : mBinder(binder), mField(field) {}

    ADataBinding<Model>* getBinder() const {
        return mBinder;
    }

    auto getField() const {
        return mField;
    }
};

/**
 * @brief Data binding implementation.
 * @tparam Model Your model type.
 * @details
 * <p>
 * If const reference of your model passed, ADataBinding will create and manage its own copy of your model.
 * </p>
 * <p>
 * If pointer of your model passed, ADataBinding will reference to your model and write directly to your model. When
 * ADataBinding is destructed the pointer will not be deleted.
 * </p>
 *
 * <p>Example:</p>
 * @code{cpp}
 * _new<ATextField>() && dataBinding(&User::username)
 * @endcode
 * <p>This code will bind ATextField with username field in the User model.</p>
 *
 * <p>Another example:</p>
 * <img src="https://github.com/aui-framework/aui/raw/develop/docs/imgs/Screenshot_20230705_173329.png">
 * <img src="https://github.com/aui-framework/aui/raw/develop/docs/imgs/Recording_20230705_at_17.51.14.gif">
 * @code{cpp}
 * class MyWindow: public AWindow {
 * public:
 *     MyWindow(): AWindow("Test") {
 *
 *         struct Model {
 *             int value = 0;
 *         };
 *
 *         auto data = _new<ADataBinding<Model>>(Model{});
 *
 *         data->addObserver(&Model::value, [](int v) {
 *             ALogger::info("Debug") << "New value: " << v;
 *         });
 *
 *         setContents(Centered {
 *             Vertical {
 *                 Label { } let {
 *                     data->addObserver(&Model::value, [it](int v) {
 *                         it->setText("{}"_format(v));
 *                     });
 *                 },
 *                 Horizontal{
 *                     Button{"+"}.clicked(this, [data] {
 *                         data->getEditableModel().value += 1;
 *                         data->notifyUpdate();
 *                     }),
 *                     Button{"-"}.clicked(this, [data] {
 *                         data->getEditableModel().value -= 1;
 *                         data->notifyUpdate();
 *                     }),
 *                 },
 *             }
 *         });
 *     }
 * };
 * @endcode
 * <p>Here, we use getEditableModel() in order to change data in our model and notifyUpdate() to notify.</p>
 * <p>Also, we use @ref #let construction to define custom format for label.</p>
 *
 */
template <typename Model>
class ADataBinding: public AObject {
private:
    using Observer = std::function<void(const Model& model, unsigned)>;
    ADeque<Observer> mLinkObservers;

    Model* mModel = nullptr;
    bool mOwning = false;

    void* mExcept = nullptr;

public:

    ADataBinding() = default;
    explicit ADataBinding(const Model& m)
    {
        setModel(m);
    }
    explicit ADataBinding(Model* m)
    {
        setModel(m);
    }

    virtual ~ADataBinding() {
        if (mOwning) {
            delete mModel;
        }
    }
    template<typename View, typename ModelField, typename SetterArg>
    ADataBindingLinker<Model, View, std::decay_t<ModelField>, std::decay_t<ModelField>, SetterArg> operator()(ModelField(Model::*field), void(View::*setterFunc)(SetterArg)) {
        AUI_ASSERT(setterFunc != nullptr);
        return ADataBindingLinker<Model, View, std::decay_t<ModelField>, std::decay_t<ModelField>, SetterArg>(this, nullptr, setterFunc, field);
    }
    template<typename View, typename ModelField, typename GetterRV, typename SetterArg>
    ADataBindingLinker<Model, View, std::decay_t<ModelField>, GetterRV, SetterArg> operator()(ModelField(Model::*field),
                                                                                              ASignal<GetterRV>(View::*getter),
                                                                                              void(View::*setterFunc)(SetterArg) = nullptr) {
        AUI_ASSERT((getter != nullptr || setterFunc != nullptr) &&
               "implement ADataBindingDefault for your view in order to use default binding");

        return ADataBindingLinker<Model, View, std::decay_t<ModelField>, GetterRV, SetterArg>(this, getter, setterFunc, field);
    }

    template<typename Data>
    ADataBindingLinker2<Model, Data> operator()(Data(Model::*field)) {
        return ADataBindingLinker2<Model, Data>(this, field);
    }
    const Model& getModel() const noexcept {
        return *mModel;
    }

    Model const * operator->() const noexcept {
        return &getModel();
    }

    Model& getEditableModel() {
        return *mModel;
    }
    void setModel(const Model& model) {
        if (mOwning) {
            delete mModel;
        }
        mOwning = true;
        mModel = new Model(model);
        notifyUpdate();
    }

    void setModel(Model* model) {
        if (mOwning) {
            delete mModel;
        }
        mOwning = false;
        mModel = model;
        notifyUpdate();
    }

    const void* getExclusion() const {
        return mExcept;
    }

    void notifyUpdate(void* except = nullptr, unsigned field = -1) {
        mExcept = except;
        for (auto& applier : mLinkObservers) {
            applier(*mModel, field);
        }
        emit modelChanged;
    }

    template<typename ModelField>
    void notifyUpdate(ModelField(Model::*field)) {
        union converter {
            unsigned i;
            decltype(field) p;
        } c;
        c.p = field;
        notifyUpdate(nullptr, c.i);
    }

    template<typename ModelField, aui::convertible_to<ModelField> U>
    void setValue(ModelField(Model::*field), U&& value) {
        mModel->*field = std::move(value);
        notifyUpdate(field);
    }

    void addObserver(Observer applier) {
        mLinkObservers << std::move(applier);
        if (mModel) {
            mLinkObservers.last()(*mModel, -1);
        }
    }

    template<aui::invocable T>
    void addObserver(T&& applier) {
        addObserver([applier = std::forward<T>(applier)](const Model&, unsigned) {
            applier();
        });
    }

    template<typename ModelField, typename FieldObserver>
    void addObserver(ModelField(Model::*field), FieldObserver&& observer) {
        mLinkObservers << [observer = std::forward<FieldObserver>(observer), field] (const Model& model, unsigned index) {
            union converter {
                unsigned i;
                decltype(field) p;
            } c;
            c.p = field;
            if (c.i == index || index == -1) {
                observer(model.*field);
            }
        };
        if (mModel) {
            mLinkObservers.last()(*mModel, -1);
        }
    }


signals:
    /**
     * @brief Data in the model has changed.
     */
    emits<> modelChanged;
};

template<typename Klass1, typename Klass2, typename Model, typename ModelField, typename GetterRV, typename SetterArg>
_<Klass1> operator&&(const _<Klass1>& object, const ADataBindingLinker<Model, Klass2, ModelField, GetterRV, SetterArg>& linker) {
    union converter {
        unsigned i;
        decltype(linker.getField()) p;
    };
    if (linker.getGetter()) {
        AObject::connect(object.get()->*(linker.getGetter()), linker.getBinder(), [object, linker](const GetterRV& data) {
            AUI_ASSERTX(&linker.getBinder()->getEditableModel(), "please setModel for ADataBinding");
            object->setSignalsEnabled(false);
            linker.getBinder()->getEditableModel().*(linker.getField()) = data;
            converter c;
            c.p = linker.getField();
            linker.getBinder()->notifyUpdate(object.get(), c.i);
            object->setSignalsEnabled(true);
        });
    }

    if (linker.getSetterFunc()) {
        linker.getBinder()->addObserver([object, linker](const Model& model, unsigned field) {
            converter c;
            c.p = linker.getField();
            if (c.i == field || field == -1) {
                if (object.get() != linker.getBinder()->getExclusion()) {
                    (object.get()->*(linker.getSetterFunc()))(model.*(linker.getField()));
                }
            }
        });
    }

    return object;
}


namespace aui::detail {
    template<typename ForcedClazz, typename Type>
    struct pointer_to_member {
        template<typename... Args>
        static Type(ForcedClazz::*with_args(std::tuple<Args...>))(Args...) {
            return nullptr;
        }
    };
}

template<typename View, typename Model, typename Data>
_<View> operator&&(const _<View>& object, const ADataBindingLinker2<Model, Data>& linker) {
    ADataBindingDefault<View, Data>::setup(object);

    using setter = aui::member<decltype(ADataBindingDefault<View, Data>::getSetter())>;

    using setter_ret = typename setter::return_t;
    using setter_args = typename setter::args;

    using my_pointer_to_member = typename aui::detail::pointer_to_member<View, setter_ret>;

    using pointer_to_setter = decltype( my_pointer_to_member::with_args(std::declval<setter_args>()));

    object && (*linker.getBinder())(linker.getField(),
                                    (ASignal<Data>(View::*))(ADataBindingDefault<View, Data>::getGetter()),
                                    static_cast<pointer_to_setter>(ADataBindingDefault<View, Data>::getSetter()));
    return object;
}
