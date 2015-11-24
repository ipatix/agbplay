#include "MyException.h"

using namespace agbplay;

MyException::MyException(const char msg[]) {
    this->msg = msg;
}

MyException::~MyException() {

}

const char *MyException::what() const throw() {
    return msg;
}
