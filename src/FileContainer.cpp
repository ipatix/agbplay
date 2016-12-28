#include <fstream>
#include <cstring>

#include "FileContainer.h"
#include "Xcept.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

FileContainer::FileContainer(string path)
{
    // load file with ifstream
    ifstream is(path, ios_base::binary);
    if (!is.is_open()) {
        throw Xcept("Error while opening ROM: %s", strerror(errno));
    }
    is.seekg(0, ios_base::end);
    long size = is.tellg();
    is.seekg(0, ios_base::beg);
    data.resize(size_t(size));
    
    // copy file to memory
    is.read((char *)data.data(), size);
    if (is.bad())
        throw Xcept("read bad");
    if (is.fail()) {
        throw Xcept("read fail");
    }
    is.close();
}

FileContainer::~FileContainer() 
{
}
