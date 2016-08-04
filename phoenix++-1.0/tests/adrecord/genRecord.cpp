#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

char VIEWID_CHAR[] = { '0', '1', '2', '3', '4', '5',
                    '6', '7', '8', '9', 'A',
                    'B', 'C', 'D', 'E', 'F' };

const char* STATES[] = {"AL","AK","AZ","AR","CA",
                        "CO","CT","DE","FL","GA",
                        "HI","ID","IL","IN","IA",
                        "KS","KY","LA","ME","MD",
                        "MA","MI","MN","MS","MO",
                        "MT","NE","NV","NH","NJ",
                        "NM","NY","NC","ND","OH",
                        "OK","OR","PA","RI","SC",
                        "SD","TN","TX","UT","VT",
                        "VA","WA","WV","WI","WY"};

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "usage: genRecord <file name>" << endl;
        return 1;
    }
    std::ofstream out(argv[1]);
    for (int i = 0; i < 1000; i++) {
        // gen viewid
        string viewId;
        for (int j = 0; j < 8; j++) {
            int id = rand() % 16;
            id = (id == 0 && j == 0) ? 1 : id;
            viewId.push_back(VIEWID_CHAR[id]);
        }
        // state
        string state = STATES[rand() % 50];
        // adId
        char adIdBuf[10];
        sprintf(adIdBuf, "%4d", rand() % 10000);
        // number of records
        for (int j = 0; j < rand() % 100; j++) {
            int clicks = rand() % 10;
            double revenue = (double)(rand() % 10000) / 100;
            out << viewId << "\t" << state << "\t" << adIdBuf << "\t"
                << clicks << "\t" << revenue << "\n";
        }
    }
    return 0;
}