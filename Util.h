#pragma once

#include <exception>
#include <stdexcept>

#define TRY_OOR(n) try { n ; } catch (std::exception& e) { }
