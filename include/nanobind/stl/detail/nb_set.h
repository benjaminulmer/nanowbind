#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Value_, typename Key> struct set_caster {
    NB_TYPE_CASTER(Value_, const_name("set[") + make_caster<Key>::Name + const_name("]"));

    using KeyCaster = make_caster<Key>;

    bool from_python(handle src, uint8_t flags, cleanup_list *cleanup) noexcept {
        value.clear();

        PyObject* iter = obj_iter(src.ptr());
        if (iter == nullptr) {
            PyErr_Clear();
            return false;
        }

        bool success = true;

        KeyCaster key_caster;
        PyObject *key;
        while ((key = PyIter_Next(iter))) {
            success &= key_caster.from_python(key, flags, cleanup);
            Py_DECREF(key);

            if (!success)
                break;

            value.emplace(((KeyCaster &&) key_caster).operator cast_t<Key&&>());
        }

        if (PyErr_Occurred()) {
            PyErr_Clear();
            success = false;
        }

        Py_DECREF(iter);

        return success;
    }

    template <typename T>
    static handle from_cpp(T &&src, rv_policy policy, cleanup_list *cleanup) {
        object ret = steal(PySet_New(nullptr));

        if (ret.is_valid()) {
            for (auto& key : src) {
                object k = steal(
                    KeyCaster::from_cpp(forward_like<T>(key), policy, cleanup));

                if (!k.is_valid())
                    return handle();

                if (PySet_Add(ret.ptr(), k.ptr()) != 0) {
                    PyErr_Clear();
                    return handle();
                }
            }
        } else {
            PyErr_Clear();
        }

        return ret.release();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
