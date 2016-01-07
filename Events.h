#pragma once

#include "CursesWin.h"

namespace agbplay
{
    class Events : public CursesWin
    {
        public:
            Events();
            ~Events();

            int GetKey();
            void WaitTick();
    };
}
