#include <exception>
#include <string>

namespace agbplay {
    class MyException : public std::exception {
        public:
            MyException(std::string msg);
            ~MyException() override;

            const char *what() const throw() override;
        private:
            std::string msg;
    };
}
