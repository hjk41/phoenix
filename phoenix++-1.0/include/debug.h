#pragma once

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "container.h"
#include "serialize.h"
#include "util.h"

bool __logging = false;
bool __replaying = false;
bool __performance_trace = true;

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
    virtual ~ReduceDebuggerBase() {}
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

static const char* _log_file = "reducer.trace";

// logger
template<typename KT, typename VT, class IteratorT>
class ReduceDebugger<KT, VT, IteratorT, true, false> 
    : public ReduceDebuggerBase<KT, VT, IteratorT> {
    std::ofstream _file;
    std::mutex _mutex;
    std::map<KT, std::vector<VT>> _sorted;
    using IT = ProxyIterator<VT, IteratorT>;
public:
    ReduceDebugger() : _file(_log_file, std::ios_base::binary) {}
    //ReduceDebugger() : _file(_log_file) {}
    ~ReduceDebugger() { _file.flush();  _file.close(); }
    
    IT get_iterator(const KT& key, const IteratorT& it) {
        std::lock_guard<std::mutex> l(_mutex);
        Serializer<KT>::serialize(_file, key);
        size_t s = it.num_items();
        Serializer<size_t>::serialize(_file, s);
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
    std::mutex _mutex;
    std::map<KT, std::vector<VT>> _kvs;
    using IT = ProxyIterator<VT, IteratorT>;
public:
    ReduceDebugger() {
        std::lock_guard<std::mutex> l(_mutex);
        std::ifstream file(_log_file, std::ios_base::binary);
        //std::ifstream file(_log_file);
        KT key;
        size_t size;
        // load data file
        while (file.good()) {
            Serializer<KT>::deserialize(file, key);
            if (!file.good()) {
                break;
            }
            Serializer<size_t>::deserialize(file, size);
            assert(_kvs.find(key) == _kvs.end());
            auto& vs = _kvs[key];
            vs.resize(size);
            for (size_t i = 0; i < size; i++) {
                Serializer<VT>::deserialize(file, vs[i]);
            }
        }
    }

    IT get_iterator(const KT& key, const IteratorT& it) {
        std::lock_guard<std::mutex> l(_mutex);
        assert(_kvs.find(key) != _kvs.end());
        return IT(_kvs[key]);
    }
};

class BufferedFile {
    std::ofstream _file;
    std::mutex _mutex;
    std::string _buf;
    static const size_t BufSize = 10 * 1024 * 1024;
public:
    BufferedFile(const char* path) : _file(path) {
        _buf.reserve(BufSize);
    }

    ~BufferedFile() {
        flush();
    }

    void flush() {
        _file << _buf;
        _buf.clear();
    }

    void operator << (const char* str) {
        std::lock_guard<std::mutex> l(_mutex);
        _buf.append(str);
        if (_buf.size() > BufSize - 1024) {
            flush();
        }
    }
};

template<typename T>
class ReduceTracer{
public:
    static void print(int workerId, const T& key, const char* prompt, double time, char* buf) {
        sprintf(buf, "%f: worker[%d]: reduce[%s]: %s\n", time, workerId, std::to_string(key).c_str(), prompt);
    }
};

template<>
class ReduceTracer<std::string> {
public:
    static void print(int workerId, const std::string& key, const char* prompt, double time, char* buf) {
        sprintf(buf, "%f: worker[%d]: reduce[%s]: %s\n", time, workerId, key.c_str(), prompt);
    }
};

template<>
class ReduceTracer<int> {
public:
    static void print(int workerId, const int& key, const char* prompt, double time, char* buf) {
        sprintf(buf, "%f: workerId[%d]: reduce[%d]: %s\n", time, workerId, key, prompt);
    }
};

class PerformanceTracer {
    static BufferedFile _file;
    static double time_start;
public:
    PerformanceTracer() {
        PerformanceTracer::master_thread_trace("program_begin");
    }

    ~PerformanceTracer() {
        PerformanceTracer::master_thread_trace("program_end");
    }

    inline static void master_thread_trace(const char* prompt) {
        if (!__performance_trace) return;
        char buf[1024];
        sprintf(buf, "%f: master_thread: %s\n", get_elapsed_time(), prompt);
        _file << buf;
    }

    inline static void worker_thread_trace(int workerId, const char* prompt) {
        if (!__performance_trace) return;
        char buf[1024];
        sprintf(buf, "%f: worker_thread[%d]: %s\n", get_elapsed_time(), workerId, prompt);
        _file << buf;
    }

    inline static void map_trace(int workerId, int mapId, const char* prompt) {
        if (!__performance_trace) return;
        char buf[1024];
        sprintf(buf, "%f: worker[%d]: map[%d]: %s\n", 
            get_elapsed_time(), workerId, mapId, prompt);
        _file << buf;
    }

    template<class KT>
    inline static void reduce_trace(int workerId, const KT& key, const char* prompt) {
        if (!__performance_trace) return;
        char buf[1024];
        ReduceTracer<KT>::print(workerId, key, prompt, get_elapsed_time(), buf);
        _file << buf;
    }

    inline static void merge_trace(int workerId, int mergeId, const char* prompt) {
        if (!__performance_trace) return;
        char buf[1024];
        sprintf(buf, "%f: worker[%d]: merge[%d]: %s\n", get_elapsed_time(), workerId, mergeId, prompt);
        _file << buf;
    }
private:
    inline static double get_elapsed_time() {
        return my_get_time() - time_start;
    }
};

BufferedFile PerformanceTracer::_file("performance.trace");
double PerformanceTracer::time_start = my_get_time();
PerformanceTracer __dummyPerformanceTracer;