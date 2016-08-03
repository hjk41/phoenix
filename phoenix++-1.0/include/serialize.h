#pragma once

#include <cassert>
#include <iostream>
#include <type_traits>
#include <typeinfo>

template<typename T, typename T2 = T>
class Serializer {
public:
    static std::ostream& serialize(std::ostream& os, const T& d) {
        std::cout << "serialize not implemented for "
            << typeid(d).name() << std::endl;
        assert(false);
        return os;
    }

    static std::istream& deserialize(std::istream& is, T& d) {
        std::cout << "dserialize not implemented for "
            << typeid(d).name() << std::endl;
        assert(false);
        return is;
    }
};

template<typename T>
class Serializer<T,
    typename std::enable_if<std::is_pod<T>::value, T>::type> {
public:
    static std::ostream& serialize(std::ostream& os, const T& d) {
        os.write((const char*)&d, sizeof(T));
        return os;
    }

    static std::istream& deserialize(std::istream& is, T& d) {
        is.read((char*)&d, sizeof(T));
        return is;
    }
};

template<typename T, typename T2 = T>
class TextSerializer {
public:
    static std::ostream& serialize(std::ostream& os, const T& d) {
        std::cout << "serialize not implemented for "
            << typeid(d).name() << std::endl;
        assert(false);
        return os;
    }

    static std::istream& deserialize(std::istream& is, T& d) {
        std::cout << "dserialize not implemented for "
            << typeid(d).name() << std::endl;
        assert(false);
        return is;
    }
};

template<typename T>
class TextSerializer<T,
    typename std::enable_if<std::is_pod<T>::value, T>::type> {
public:
    static std::ostream& serialize(std::ostream& os, const T& d) {
        os << d << std::endl;
        return os;
    }

    static std::istream& deserialize(std::istream& is, T& d) {
        is >> d;
        return is;
    }
};