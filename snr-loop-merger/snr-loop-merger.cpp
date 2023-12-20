#include <iostream>
#include <fstream>
#include <stdint.h>
using namespace std;

#define LOOP_FLAG_OFFSET 0x4
#define LOOP_FLAG_VALUE 0x20
#define SEGMENT_SIZE_OFFSET 0x5
#define LOOP_CUT_OFFSET 0x8

uint32_t ReadSegmentSize(ifstream& stream) { // uint24_t does not exists
    stream.seekg(SEGMENT_SIZE_OFFSET);

    char segmentSizeBytes[4];
    for (size_t i = 0; i < 4; i++)
    {
        stream.read(&segmentSizeBytes[i], sizeof(segmentSizeBytes[0]));
    }

    return uint8_t(segmentSizeBytes[2])
        + (uint8_t(segmentSizeBytes[1]) << 8)
        + (uint8_t(segmentSizeBytes[0]) << 16);
};

uint32_t InvertEndian24(uint32_t valueToInvert)
{
    uint8_t* bytes = (uint8_t*)&valueToInvert;

    return (bytes[0] << 16)
        + (bytes[1] << 8)
        + bytes[2];
}

void ProcessSegment(ifstream& inputStream, fstream& outputStream, uint32_t offset) {
    inputStream.seekg(0, inputStream.end);
    int length = (int)inputStream.tellg() - offset;
    inputStream.seekg(offset);
    char* buffer = new char[length];
    inputStream.read(buffer, length);
    outputStream.write(buffer, length);
    delete[] buffer;
}

int main()
{
    std::fstream output("output.snr", ios_base::out | std::ios::binary);

    ifstream partA("initial_part.snr", std::ios::binary);
    if (!partA.is_open()) {
        printf("Could not open 'initial_part.snr'. Terminating....");
        system("pause");
        return 0;
    }

    ifstream partB("loop_part.snr", std::ios::binary);
    if (!partB.is_open()) {
        printf("Could not open 'loop_part.snr'. Terminating....");
        system("pause");
        return 0;
    }

    partA.seekg(0);
    output.seekg(0);
    uint32_t header = 0;
    partA.read((char*)(&header), sizeof(header));
    output.write((char*)&header, sizeof(header));

    char flags; // ? 0x20 - isLooped, something else?
    partA.read(&flags, sizeof(flags));
    flags += LOOP_FLAG_VALUE;
    output.write(&flags, sizeof(flags));

    uint32_t fullAudioSegmentSize = ReadSegmentSize(partA);
    fullAudioSegmentSize += ReadSegmentSize(partB);
    uint32_t invertedAudioSegmentSize = InvertEndian24(fullAudioSegmentSize);
    output.write((char*)&invertedAudioSegmentSize, sizeof(flags) * 3);

    std::cout << "Processing initial segment." << endl;
    ProcessSegment(partA, output, LOOP_FLAG_OFFSET);

    std::cout << "Processing loop segment." << endl;
    ProcessSegment(partB, output, LOOP_CUT_OFFSET);

    partA.close();
    partB.close();
    output.close();

    std::cout << "Finished." << endl;
    system("pause");
    return 0;
}