#include <exception>
#include <string>

#define MAX_EXCEPTION_LENGTH 1024

class Xcept : public std::exception {
    public:
        Xcept(const char *format, ...);
        ~Xcept() override;

        const char *what() const throw() override;
    private:
        std::string msg;
};
