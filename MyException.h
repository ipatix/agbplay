#include <exception>

namespace agbplay {
    class MyException : public std::exception {
        public:
            MyException(const char msg[]);
            ~MyException() override;

            const char *what() const throw() override;
        private:
            const char *msg;
    };
}
