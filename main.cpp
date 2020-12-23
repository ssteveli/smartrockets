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
float maxForce = 0.5f;
float maxSpeed = 3.0f;

void max(olc::vf2d &v, float max)
{
    v.norm() *= max;
}

void limit(olc::vf2d &v, float max)
{
    const float mSq = v.mag();
    if (mSq > max * max)
    {
        v /= sqrt(mSq);
        v *= max;
    }
}

class SmartRockets : public olc::PixelGameEngine
{
private:
    int numberOfRockets = 10;
    int iterations = 400;
    int frame = 0;

    olc::Sprite *sprRocket = NULL;
    olc::Decal *dRocket = NULL;

    struct DNA
    {
        std::vector<olc::vf2d> genes;
    };

    struct Rocket
    {
        olc::vf2d pos;
        olc::vf2d vel;
        olc::vf2d acc;
        DNA dna;
        bool completed;
        bool crashed;

        void applyForce(olc::vf2d force)
        {
            acc += force;
        }

        void update()
        {
            vel += acc;
            limit(vel, maxSpeed);
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
            rocket.vel = {0.0f, 0.0f};
            rocket.acc = {0.0f, 0.0f};
            rocket.completed = false;
            rocket.crashed = false;

            for (int n = 0; n < iterations; n++)
            {
                rocket.dna.genes.push_back({distr(eng), distr(eng)});
            }

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
            if (frame++ >= iterations)
            {
                rocket.pos = {ScreenWidth() / 2.0f, ScreenHeight() - 50.0f};
                frame = 0;
            }

            if (!rocket.completed || rocket.crashed)
            {
                rocket.applyForce(rocket.dna.genes[frame]);
                rocket.update();
            }

            DrawRotatedDecal(
                rocket.pos,
                dRocket,
                0.2f,
                {sprRocket->width / 2.0f, sprRocket->height / 2.0f});
        }
        SetPixelMode(olc::Pixel::NORMAL);
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
