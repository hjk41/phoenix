#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "container.h"
#include "serialize.h"

template<typename T>
class LoggedIterator {
public:
    LoggedIterator(const std::vector<T>& values)
        : _values(values) {
        _it = _values.begin();
    }

    bool next(T& v) const {
        if (_it != _values.end()) {
            v = *_it;
            ++_it;
            return true;
        }
        else {
            return false;
        }
    }

    size_t size() const {
        return _values.size();
    }
private:
    std::vector<T> _values;
    mutable typename std::vector<T>::iterator _it;
};

template<typename VT, 
    typename IteratorT>
class ProxyIterator {
public:
    ProxyIterator(const IteratorT& it) {
        _it1 = &it;
    }

    ProxyIterator(const std::vector<VT>& values) {
        _it2 = new LoggedIterator<VT>(values);
    }

    ~ProxyIterator() {
        delete _it2;
    }

    bool next(VT& v) const {
        return _it1 ? _it1->next(v) : _it2->next(v);
    }

    size_t size() const {
        return _it1 ? _it1->size() : _it2->size();
    }
private:
    const IteratorT* _it1 = nullptr;
    LoggedIterator<VT>* _it2 = nullptr;
};

template<typename KT, typename VT, class IteratorT>
class ReduceDebuggerBase {
    using IT = ProxyIterator<VT, IteratorT>;
public:
    virtual IT get_iterator(const KT& key, const IteratorT& it) = 0;
};

template<typename KT, typename VT, class IteratorT, 
    bool DoLogging, bool DoReplaying>
class ReduceDebugger : public ReduceDebuggerBase<KT, VT, IteratorT>{
};

// normal execution
template<typename KT, typename VT, class IteratorT>
class ReduceDebugger<KT, VT, IteratorT, false, false> 
    : public ReduceDebuggerBase<KT, VT, IteratorT> {
    using IT = ProxyIterator<VT, IteratorT>;
public:
    IT get_iterator(const KT& key, const IteratorT& it) {
        return IT(it);
    }
};

// logger
template<typename KT, typename VT, class IteratorT>
class ReduceDebugger<KT, VT, IteratorT, true, false> 
    : public ReduceDebuggerBase<KT, VT, IteratorT> {
    const std::string& _log_file = ".debug_data";
    std::ofstream _file;
    std::mutex _mutex;
    using IT = ProxyIterator<VT, IteratorT>;
public:
    ReduceDebugger() : _file(_log_file, std::ios_base::binary) {}
    
    IT get_iterator(const KT& key, const IteratorT& it) {
        std::lock_guard<std::mutex> l(_mutex);
        Serializer<KT>::serialize(_file, key);
        Serializer<size_t>::serialize(_file, it.size());
        IteratorT tmp = it;
        VT v;
        while (tmp.next(v)) {
            Serializer<VT>::serialize(_file, v);
        }
        return IT(it);
    }
};

// replayer
template<typename KT, typename VT, class IteratorT>
class ReduceDebugger<KT, VT, IteratorT, false, true> 
    : public ReduceDebuggerBase<KT, VT, IteratorT> {
    const std::string& _log_file = ".debug_data";
    std::ifstream _file;
    std::mutex _mutex;
    std::map<KT, std::vector<VT>> _kvs;
    using IT = ProxyIterator<VT, IteratorT>;
public:
    ReduceDebugger() : _file(_log_file, std::ios_base::binary) {
        std::lock_guard<std::mutex> l(_mutex);
        KT key;
        size_t size;
        // load data file
        while (_file.good()) {
            Serializer<KT>::deserialize(_file, key);
            Serializer<size_t>::deserialize(_file, size);
            assert(_kvs.find(key) == _kvs.end());
            auto& vs = _kvs[key];
            vs.resize(size);
            for (size_t i = 0; i < size; i++) {
                Serializer<VT>::deserialize(_file, vs[i]);
            }
        }
    }

    IT get_iterator(const KT& key, const IteratorT& it) {
        std::lock_guard<std::mutex> l(_mutex);
        assert(_kvs.find(key) != _kvs.end());
        return IT(_kvs[key]);
    }
};