#include <iostream>
#include <fstream>
#include <stdint.h>
using namespace std;

#define LOOP_FLAG_OFFSET 0x4
#define LOOP_FLAG_VALUE 0x20
#define SEGMENT_SIZE_OFFSET 0x5
#define LOOP_CUT_OFFSET 0x8

struct SArguments {
    SArguments() :
        ShowInfo(false),
        loop(""),
        input(""),
        output("")
    {
    };
    bool ShowInfo;
    string loop;
    string input;
    string output;
};

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

bool ParseArgs(SArguments& Args, unsigned long Argc, char* Argv[]) {
    
    for (uint32_t i = 1; i < Argc;) {
        string Arg(Argv[i++]);
        if (Arg == "-o" || Arg == "--output") {
            if (i >= Argc) {
                return false;
            }
            Args.output = string(Argv[i++]);
        }
        else if (Arg == "-l" || Arg == "--loop") {
            if (i >= Argc) {
                return false;
            }
            Args.loop = string(Argv[i++]);
        }
        else {
            Args.input = Arg;
        }
    }
    return true;
}

void ShowInfo(const string& Program) {
    cout << "Usage: " << Program << " InputFilename [Options] " << endl;
    cout << "-l, --loop    Loop part    The music track that will loop after the initial part." << endl;
    cout << "-o, --output  File         Specify the output filename (.snr)." << endl;
}

void ProcessLoopMerge(SArguments& Args) {
    ifstream partA(Args.input.c_str(), std::ios_base::in | std::ios::binary);
    ifstream partB(Args.loop.c_str(), std::ios_base::in | std::ios::binary);
    std::fstream output(Args.output, ios_base::out | std::ios::binary);

    if (!partA.is_open()) {
       printf("Could not open the initial part. Terminating....");
       return;
    }
    if (!partB.is_open()) {
       printf("Could not open the loop part. Terminating....");
       return;
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
}

void ProcessLooping(SArguments& Args) {
    ifstream partA(Args.input.c_str(), std::ios_base::in | std::ios::binary);
    std::fstream output(Args.output, ios_base::out | std::ios::binary);

    if (!partA.is_open()) {
        printf("Could not open the initial part. Terminating....");
        return;
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
    
    uint32_t segmentSize = 0;
    partA.read((char*)(&segmentSize), sizeof(flags) * 3);
    output.write((char*)&segmentSize, sizeof(flags) * 3);

    uint32_t padding = 0;
    output.write((char*)&padding,sizeof(padding));

    std::cout << "Processing initial segment." << endl;
    ProcessSegment(partA, output, LOOP_CUT_OFFSET);

    partA.close();
    
    output.close();

    std::cout << "Finished." << endl;
}

int main(int Argc, char** Argv)
{
    SArguments Args;

    if (Argc == 1) {
        Args.ShowInfo = true;
    }

    bool parse = ParseArgs(Args,Argc,Argv);

    if (Args.ShowInfo) {
        ShowInfo(Argv[0]);
    }

    if (!parse) {
        printf("Could not parse arguments. Terminating....");
        system("pause");
        return 0;
    }

    if (Args.input.empty()) {
        cerr << "You must specify an input filename." << std::endl;
        system("pause");
        return 0;
    }

    if (Args.output.empty())
    {
        cerr << "You must specify an output filename." << std::endl;
        system("pause");
        return 0;
    }

    if (Args.loop.empty()) {
        ProcessLooping(Args);
    }
    else {
        ProcessLoopMerge(Args);
    }
   
    system("pause");
    return 0;
}