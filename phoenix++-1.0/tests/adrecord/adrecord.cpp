#include <fstream>
#include <string>
#include <vector>

#include "combiner.h"
#include "map_reduce.h"
#include "debug.h"
#include "util.h"

using namespace std;
struct AdRecord {
    string ViewID;
    string State;
    string AdId;
    uint64_t Clicks;
    double Revenue;
};

template<>
class TextSerializer<AdRecord> {
public:
    static std::ostream& serialize(std::ostream& os, const AdRecord& d) {
        TextSerializer<string>::serialize(os, d.ViewID);
        TextSerializer<string>::serialize(os, d.State);
        TextSerializer<string>::serialize(os, d.AdId);
        TextSerializer<uint64_t>::serialize(os, d.Clicks);
        TextSerializer<double>::serialize(os, d.Revenue);
        return os;
    }

    static std::istream& deserialize(std::istream& is, AdRecord& d) {
        TextSerializer<string>::deserialize(is, d.ViewID);
        TextSerializer<string>::deserialize(is, d.State);
        TextSerializer<string>::deserialize(is, d.AdId);
        TextSerializer<uint64_t>::deserialize(is, d.Clicks);
        TextSerializer<double>::deserialize(is, d.Revenue);
        return is;
    }
};

template<>
class Serializer<AdRecord, AdRecord> {
public:
    static std::ostream& serialize(std::ostream& os, const AdRecord& d) {
        Serializer<string>::serialize(os, d.ViewID);
        Serializer<string>::serialize(os, d.State);
        Serializer<string>::serialize(os, d.AdId);
        Serializer<uint64_t>::serialize(os, d.Clicks);
        Serializer<double>::serialize(os, d.Revenue);
        return os;
    }

    static std::istream& deserialize(std::istream& is, AdRecord& d) {
        Serializer<string>::deserialize(is, d.ViewID);
        Serializer<string>::deserialize(is, d.State);
        Serializer<string>::deserialize(is, d.AdId);
        Serializer<uint64_t>::deserialize(is, d.Clicks);
        Serializer<double>::deserialize(is, d.Revenue);
        return is;
    }
};

class HistogramMR : public MapReduceSort<HistogramMR, 
    string,
    string, 
    AdRecord, 
    hash_container<string, AdRecord, buffer_combiner>>
{
public:
    void map(data_type const& p, map_container& out) const {
        // parse a line into a record and emit
        vector<string> parts = split_str(p, "\t");
        if (parts.size() != 5) {
            cout << "error parsing record: " << p << endl;
        }
        assert(parts.size() == 5);
        string& viewId = parts[0];
        string& state = parts[1];
        string& adId = parts[2];
        uint64_t clicks;
        double revenue;
        str2T(parts[3], clicks);
        str2T(parts[4], revenue);
        AdRecord record = {viewId, state, adId, clicks, revenue};
        emit_intermediate(out, viewId, record);
    }

    void reduce(key_type const& key, reduce_iterator const& values, std::vector<keyval>& out) const {
        value_type val;
        uint64_t clicks = 0;
        double revenue = 0;
        string viewId, state, adId;
        while (values.next(val))
        {
            viewId = val.ViewID;
            state = val.State;
            adId = val.AdId;
            clicks += val.Clicks;
            revenue += val.Revenue;
        }
        AdRecord result;
        result.ViewID = viewId;
        result.State = state;
        result.AdId = adId;
        result.Clicks = clicks;
        result.Revenue = revenue;
        keyval kv = { key, result };
        out.push_back(kv);
    }
};

int main(int argc, char *argv[]) {
    timespec begin, end;
    get_time(begin);

    // Make sure a filename is specified
    if (argv[1] == NULL)
    {
        printf("USAGE: %s <record filename> <log/replay>\n", argv[0]);
        exit(1);
    }

    string fname = argv[1];
    printf("ADRecord: Running...\n");
    ifstream in(fname);
    vector<string> input;
    string line;
    while (in.good()) {
        getline(in, line);
        if (!line.empty()) {
            input.push_back(line);
        }        
    }    
    printf("This file has %d records\n", input.size());
    get_time(end);
    print_time("initialize", begin, end);

    fprintf(stderr, "ADRecord: Calling MapReduce Scheduler\n");
    get_time(begin);
    std::vector<HistogramMR::keyval> result;
    HistogramMR* mapReduce = new HistogramMR();
    CHECK_ERROR(mapReduce->run(input.data(), input.size(), result) < 0);
    delete mapReduce;
    get_time(end);
    print_time("library", begin, end);

    get_time(begin);
    for (size_t i = 0; i < 10 && i < result.size(); i++)
    {
        AdRecord& r = result[i].val;
        cout << r.ViewID << "\t" 
            << r.State << "\t" 
            << r.AdId << "\t" 
            << r.Clicks << "\t" 
            << r.Revenue << endl;
    }
    get_time(end);
    print_time("finalize", begin, end);

    return 0;
}

// vim: ts=8 sw=4 sts=4 smarttab smartindent
