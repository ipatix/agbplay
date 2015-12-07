#include <fstream>

#include "FileContainer.h"

using namespace std;
using namespace agbplay;

FileContainer::FileContainer(string path)
{
    // load file with ifstream
    ifstream is(path);
    is.seekg(0, ios_base::end);
    long size = is.tellg();
    is.seekg(0, ios_base::beg);
    data = vector<uint8_t>((size_t)size, 0);
    
    // copy file to memory
    is.read((char *)data.data(), size);
    is.close();
}

FileContainer::~FileContainer() {

}
