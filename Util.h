#pragma once

#include <stdexcept>

#define TRY_OOR(n) try { n ; } catch (std::out_of_range& e) { }
