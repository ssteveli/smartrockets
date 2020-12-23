#include <iostream>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine/olcPixelGameEngine.h"

class SmartRockets : public olc::PixelGameEngine
{
public:
    SmartRockets()
    {
        sAppName = "SmartRockets";
    }

    bool OnUserCreate() override
    {
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        return true;
    }
};

int main(int, char **)
{
    SmartRockets SmartRockets;
    if (SmartRockets.Construct(800, 800, 1, 1))
        SmartRockets.Start();

    return 0;
}
