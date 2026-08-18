#include <unordered_set>
using namespace std;

int countDistinctDiagnosticWindows(int input1[], int input2, int input3, int input4) {
    int n = input2, k = input3, p = input4;

    const unsigned long long M1 = 1000000007ULL, M2 = 998244353ULL;
    const unsigned long long B1 = 131ULL, B2 = 137ULL;

    unordered_set<unsigned long long> seen;

    for (int i = 0; i < n; i++) {
        unsigned long long h1 = 0, h2 = 0;
        int cnt = 0;

        for (int j = i; j < n; j++) {
            if (input1[j] % p == 0) cnt++;
            if (cnt > k) break;

            unsigned long long v = (unsigned long long)input1[j] + 1;
            h1 = (h1 * B1 + v) % M1;
            h2 = (h2 * B2 + v) % M2;

            seen.insert(h1 * M2 + h2);
        }
    }

    return seen.size();
}


