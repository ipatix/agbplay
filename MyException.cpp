#include "MyException.h"

using namespace agbplay;
using namespace std;

MyException::MyException(string msg) {
    this->msg = msg;
}

MyException::~MyException() {

}

const char *MyException::what() const throw() {
    return msg.c_str();
}
