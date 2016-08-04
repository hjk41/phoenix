#pragma once
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

inline std::string strip(const std::string & str, char c) {
    std::string ret;
    ret.reserve(str.size());
    int i = 0;
    while (str[i] == c) i++;
    size_t j = str.size() - 1;
    while (str[j] == c) j--;
    ret.assign(str.data() + i, str.data() + j + 1);
    return ret;
}

inline std::string to_lower(const std::string & str1)
{
    std::string str(str1);
    for (auto & c : str) {
        c = tolower(c);
    }
    return str;
}

inline std::vector<std::string> split_str(const std::string & str)
{
    using namespace std;
    istringstream iss(str);
    vector<std::string> substrings;
    copy(std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>(),
        std::back_inserter<std::vector<std::string>>(substrings));
    return substrings;
}

inline std::vector<std::string> split_str(const std::string & str, const std::string & splitter)
{
    using namespace std;
    vector<std::string> substrings;
    string::size_type start_pos = 0;
    while (start_pos < str.size())
    {
        string::size_type end = str.find(splitter, start_pos);
        if (end > start_pos)
            substrings.push_back(str.substr(start_pos, end - start_pos));
        if (end == string::npos)
            break;
        start_pos = end + splitter.size();
    }
    return substrings;
}

template<class T>
class String2Type
{
public:
    static void Get(const std::string & str, T & n)
    {
        std::istringstream iss(str);
        iss >> n;
    }
};

template<>
class String2Type<uint64_t>
{
public:
    static void Get(const std::string & str, uint64_t & n)
    {
#ifdef WIN32
        sscanf_s(str.c_str(), "%llu", &n);
#else
        sscanf(str.c_str(), "%llu", &n);
#endif
    }
};

template<>
class String2Type<int>
{
public:
    static void Get(const std::string & str, int & n)
    {
#ifdef WIN32
        sscanf_s(str.c_str(), "%d", &n);
#else
        sscanf(str.c_str(), "%d", &n);
#endif
    }
};

template<class T>
void str2T(const std::string & str, T & n)
{
    String2Type<T>::Get(str, n);
};

inline bool starts_with(const std::string & str, const std::string & pattern)
{
    return str.size() >= pattern.size() && str.substr(0, pattern.size()) == pattern;
}

template<class T>
inline std::ostream & ostream_write(std::ostream & os, const T & v)
{
    os.write((const char*)&v, sizeof(v));
    return os;
}

template<class T>
inline std::istream & istream_read(std::istream & is, T & v)
{
    is.read((char*)&v, sizeof(v));
    return is;
}

inline double my_get_time()
{
    using namespace std::chrono;
    high_resolution_clock::duration tp = high_resolution_clock::now().time_since_epoch();
    return (double)tp.count() * high_resolution_clock::period::num / high_resolution_clock::period::den;
}

inline void my_sleep_us(uint64_t us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

