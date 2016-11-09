#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>
#include <algorithm>

using namespace std;

typedef unsigned long long LONG_8;

namespace {

    const string IN = "input.bin";
    const string OUT = "output.bin";
    const string TEMP = "output-temp.bin";

//    const string IN = "/Users/akasiyanik/FPMI/Tolstikov/io-merge-sort/in.bin";
//    const string TEMP = "/Users/akasiyanik/FPMI/Tolstikov/io-merge-sort/out-temp.bin";
//    const string OUT = "/Users/akasiyanik/FPMI/Tolstikov/io-merge-sort/out.bin";

    const int MEMORY = 10000;

//    ___________________________TEST

//    void generateInFile(LONG_8 n) {
//        ofstream file(IN, ios::out | ios::binary);
//        if (file.is_open()) {
//            file.write((char *) &n, sizeof(n));
//            for (int i = 0; i < n; i++) {
//                LONG_8 num = rand();
//                num = (num << 32) | rand();
//                file.write((char *) &num, sizeof(num));
//            }
//            file.close();
//        }
//    }
//
//    void printFile(string IN) {
//        ifstream file(IN, ios::in | ios::binary);
//        cout << "-------------" << endl;
//        if (file.is_open()) {
//            LONG_8 n;
//            file.read((char *) &n, sizeof(n));
//            LONG_8 num;
//            for (int i = 0; i < n; ++i) {
//                file.read((char *) &num, sizeof(num));
//                cout << num << " ";
//            }
//            cout << endl;
//            file.close();
//        }
//        cout << "-------------" << endl;
//    }
// _________________________________________________


    void RAMQuickSort(LONG_8 *array, int n) {
        std::sort(array, array + n);
    }

    void readBlock(ifstream &in, int block_i, int BLOCK, int curr_block_size, LONG_8 *__restrict a) {
        int startA = 8;
        int block_start_position = startA + block_i * BLOCK * 8;
        in.seekg(block_start_position, ios_base::beg);
        in.read((char *) a, curr_block_size * sizeof(LONG_8));
    }

    int writeBlock(ofstream &out, int block_i, int BLOCK, int curr_block_size, LONG_8 *__restrict a) {
        int startA = 8;
        int block_start_position = startA + block_i * BLOCK * 8;
        out.seekp(block_start_position, ios_base::beg);
        out.write((char *) a, curr_block_size * sizeof(LONG_8));
        return block_start_position;
    }

    class MergeArray {
    public:
        LONG_8 *a;
        int startBinPos; // started position of sorted array
        int size; // total elements in sorted array
        int mergedCount = 0; // processed elements

        int chunkSize; // block to read
        int mergedInChunk = 0; // current position in merge array

        MergeArray(int startBinPos, int pSize, int pChunkSize) : startBinPos(startBinPos) {
            chunkSize = pChunkSize;
            size = pSize;
            if (chunkSize > size) {
                chunkSize = size;
            }
        }

        LONG_8 getMin(ifstream &in) {
            if (mergedInChunk == chunkSize || mergedCount == 0) {
                readNewChunk(in);
            }
            LONG_8 min = a[mergedInChunk];
            mergedInChunk++;
            mergedCount++;
            return min;
        }

        void readNewChunk(ifstream &in) {
            int block_start_position = startBinPos + mergedCount * 8;
            int leftToMerge = size - mergedCount;
            if (leftToMerge < chunkSize) {
                chunkSize = leftToMerge;
            }
            if (mergedCount != 0) {
                delete[] a;
            }
            a = new LONG_8[chunkSize];
            in.seekg(block_start_position, ios_base::beg);
            in.read((char *) a, chunkSize * sizeof(LONG_8));

            mergedInChunk = 0;
        }

        bool isMerged() {
            return mergedCount == size;
        }

    };

    vector<MergeArray> InitialSort(ifstream &in, int BLOCK, int blockCount, LONG_8 n) {
        ofstream t_out(TEMP, ios::out | ios::binary);
        vector<MergeArray> mergeArrays;
        int MERGE_BLOCK = (int) ((float) MEMORY / (blockCount + 1));

        int curr_block_size;
        for (int i = 0; i < blockCount; ++i) {
            if (i != blockCount - 1 || n % BLOCK == 0) {
                curr_block_size = BLOCK;
            } else {
                curr_block_size = (int) n % BLOCK;
            }
            LONG_8 *a = new LONG_8[curr_block_size];

            readBlock(in, i, BLOCK, curr_block_size, a);
            RAMQuickSort(a, curr_block_size);
            int blockStartPosInOutBin = writeBlock(t_out, i, BLOCK, curr_block_size, a);

            mergeArrays.push_back(MergeArray(blockStartPosInOutBin, curr_block_size, MERGE_BLOCK));

            delete[] a;
        }
        t_out.close();
        return mergeArrays;
    }

    LONG_8 minValue(vector<LONG_8> &minValues, vector<MergeArray> &mergeArrays, ifstream &in) {
        LONG_8 min = minValues[0];
        int min_i = 0;
        for (int i = 0; i < minValues.size(); i++) {
            if (minValues[i] < min) {
                min = minValues[i];
                min_i = i;
            }
        }
        if (mergeArrays[min_i].isMerged()) {
            mergeArrays.erase(mergeArrays.begin() + min_i);
            minValues.erase(minValues.begin() + min_i);
        } else {
            minValues[min_i] = mergeArrays[min_i].getMin(in);
        }
        return min;
    }


    void MergeSort() {
        ifstream in(IN, ios::in | ios::binary);
        ofstream out(OUT, ios::out | ios::binary);
        if (in.is_open() && out.is_open()) {

            LONG_8 n;
            in.read((char *) &n, sizeof(n));
            out.write((char *) &n, sizeof(n));

            int blockCount = (int) ceil((float) n / MEMORY);
            int MERGE_BLOCK = (int) ((float) MEMORY / (blockCount + 1));

            vector<MergeArray> merge_arrays = InitialSort(in, MEMORY, blockCount, n);
            in.close();

            ifstream t_in(TEMP, ios::in | ios::binary);

            LONG_8 *out_buffer = new LONG_8[MERGE_BLOCK];
            int countInBuffer = 0;

            vector<LONG_8> minValues(blockCount);
            for (int i = 0; i < merge_arrays.size(); i++) {
                minValues[i] = merge_arrays[i].getMin(t_in);
            }

            while (!merge_arrays.empty()) {
                if (countInBuffer == MERGE_BLOCK) {
                    out.write((char *) out_buffer, countInBuffer * sizeof(LONG_8));
                    countInBuffer = 0;
                }
                LONG_8 min = minValue(minValues, merge_arrays, t_in);
                out_buffer[countInBuffer] = min;
                countInBuffer++;
            }
            if (countInBuffer != 0) {
                out.write((char *) out_buffer, countInBuffer * sizeof(LONG_8));
            }
            t_in.close();
            remove(TEMP.c_str());
        }
        out.close();

    }


}

int main(int argc, char *argv[]) {
//    generateInFile(5);
//
//    cout << "IN FILE " << endl;
//    printFile(IN);

    MergeSort();
//
//    cout << "OUT FILE " << endl;
//    printFile(OUT);

    return 0;
}
