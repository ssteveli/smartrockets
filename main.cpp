#include <iostream>
#include <random>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine/olcPixelGameEngine.h"

constexpr float PI = 3.14159;
constexpr float FLOAT_MIN = -5.0f;
constexpr float FLOAT_MAX = 5.0f;

std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<float> distr(FLOAT_MIN, FLOAT_MAX);

class SmartRockets : public olc::PixelGameEngine
{
private:
    int numberOfRockets = 10;
    olc::Sprite *sprRocket = NULL;
    olc::Decal *dRocket = NULL;

    struct Rocket
    {
        olc::vf2d pos;
        olc::vf2d vel;
        olc::vf2d acc;

        void applyForce(olc::vf2d force)
        {
            acc += force;
        }

        void update()
        {
            vel += acc;
            pos += vel;
            acc *= 0.0f;
        }
    };

    std::vector<Rocket> rockets;

public:
    SmartRockets()
    {
        sAppName = "SmartRockets";
    }

    bool OnUserCreate() override
    {
        sprRocket = new olc::Sprite("./assets/rocket.png");
        dRocket = new olc::Decal(sprRocket);

        for (int i = 0; i < numberOfRockets; i++)
        {
            Rocket rocket;
            rocket.pos = {ScreenWidth() / 2.0f, ScreenHeight() - 50.0f};
            rocket.vel = {distr(eng), distr(eng)};
            rocket.acc = {0.0f, 0.0f};
            rockets.push_back(rocket);
        }

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(olc::BLACK);
        SetPixelMode(olc::Pixel::ALPHA);
        for (auto &rocket : rockets)
        {
            rocket.update();

            DrawRotatedDecal(
                rocket.pos,
                dRocket,
                atan2(rocket.vel.y, rocket.vel.x) + 0.90f);
        }

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
