#pragma once

#include "CursesWin.h"

namespace agbplay
{
    class ControlGUI : public CursesWin
    {
        public:
            ControlGUI();
            ~ControlGUI();

            int GetKey();
    };
}
