#include <iostream>
#include <random>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine/olcPixelGameEngine.h"

constexpr float PI = 3.14159;
constexpr float FLOAT_MIN = -20.0f;
constexpr float FLOAT_MAX = 20.0f;

std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<float> distr(FLOAT_MIN, FLOAT_MAX);

uint8_t numberOfRockets = 100;
uint16_t lifespan = 300;
uint16_t age = 0;
uint16_t generation = 0;

float maxSpeed = 100.0f;

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

float dist(olc::vf2d &v1, olc::vf2d &v2)
{
    return sqrtf((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

class SmartRockets : public olc::PixelGameEngine
{
private:
    olc::Sprite *sprRocket = NULL;
    olc::Sprite *sprTarget = NULL;
    olc::Decal *dRocket = NULL;

    struct DNA
    {
        std::vector<olc::vf2d> genes;

        void mate(DNA other, DNA &child)
        {
            child.genes.clear();
            int mid = rand() % genes.size();
            for (int i = 0; i < genes.size(); i++)
            {
                int mutate = rand() % 100;
                if (mutate < 3)
                {
                    // mutate
                    child.genes.push_back({distr(eng), distr(eng)});
                }
                else if (i > mid)
                {
                    child.genes.push_back(other.genes[i]);
                }
                else
                {
                    child.genes.push_back(genes[i]);
                }
            }
        }
    };

    struct Rocket
    {
        olc::vf2d pos;
        olc::vf2d vel;
        olc::vf2d acc;
        float health;
        DNA dna;
        bool completed;
        bool crashed;
        uint16_t age;

        void applyForce(olc::vf2d force)
        {
            acc += force;
        }

        void update()
        {
            if (!crashed && !completed)
            {
                vel += acc;
                pos += vel;
                age++;
            }
            acc *= 0.0f;
        }
    };

    std::vector<Rocket> rockets;
    olc::vf2d target;
    olc::vf2d targetCenter;
    olc::vi2d blockerPos = {200, 400};
    olc::vi2d blockerSize = {400, 10};
    uint16_t maxWinners = 0;

    void scoreHealth()
    {
        for (auto &rocket : rockets)
        {
            float d = dist(rocket.pos, target);
            rocket.health = map(d, 0, ScreenWidth(), ScreenWidth(), 0);

            if (rocket.completed)
            {
                rocket.health *= 10;
            }
            else if (rocket.crashed)
            {
                rocket.health /= 10;
            }
        }
    }

    void repopulate()
    {
        std::vector<Rocket> genenicPool;

        float maxHealth = 0.0f;
        for (auto &rocket : rockets)
        {
            if (rocket.health > maxHealth)
            {
                maxHealth = rocket.health;
            }
        }
        std::cout << "max score: " << maxHealth << std::endl;

        // create our generic pool of rockets based on health
        for (auto &rocket : rockets)
        {
            rocket.health /= maxHealth;

            int n = rocket.health * 100;
            for (int i = 0; i < n; i++)
            {
                genenicPool.push_back(rocket);
            }
        }
        std::cout << "pool: " << genenicPool.size() << std::endl;

        rockets.clear();
        for (int i = 0; i < numberOfRockets; i++)
        {
            DNA parentA = genenicPool[rand() % genenicPool.size()].dna;
            DNA parentB = genenicPool[rand() % genenicPool.size()].dna;

            Rocket rocket;
            rocket.pos = {ScreenWidth() / 2.0f, ScreenHeight() - 50.0f};
            rocket.vel = {0.0f, 0.0f};
            rocket.acc = {0.0f, 0.0f};
            rocket.completed = false;
            rocket.crashed = false;
            rocket.health = 0;
            rocket.age = 0;
            parentA.mate(parentB, rocket.dna);
            rockets.push_back(rocket);
        }
    }

public:
    SmartRockets()
    {
        sAppName = "SmartRockets";
    }

    bool OnUserCreate() override
    {
        sprRocket = new olc::Sprite("./assets/rocket.png");

        sprTarget = new olc::Sprite("./assets/star.png");
        target = {(ScreenWidth() / 2.0f) - (sprTarget->width / 2.0f), 50.0f};
        targetCenter = {target.x + (sprTarget->width / 2), target.y - (sprTarget->height / 2)};

        dRocket = new olc::Decal(sprRocket);

        for (int i = 0; i < numberOfRockets; i++)
        {
            Rocket rocket;
            rocket.pos = {ScreenWidth() / 2.0f, ScreenHeight() - 50.0f};
            rocket.vel = {0.0f, 0.0f};
            rocket.acc = {0.0f, 0.0f};
            rocket.health = 0;
            rocket.age = 0;
            rocket.completed = false;
            rocket.crashed = false;

            for (int n = 0; n < lifespan; n++)
            {
                rocket.dna.genes.push_back({distr(eng), distr(eng)});
            }

            rockets.push_back(rocket);
        }

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        if (++age >= lifespan)
        {
            scoreHealth();
            repopulate();

            age = 0;
            generation++;
        }

        uint16_t winners = 0;
        uint16_t losers = 0;

        for (auto &rocket : rockets)
        {
            if (dist(rocket.pos, targetCenter) <= 50.0f)
            {
                rocket.completed = true;
                winners++;
            }

            if (rocket.pos.x < 0 || rocket.pos.x > ScreenWidth() || rocket.pos.y < 0 || rocket.pos.y > ScreenHeight())
            {
                rocket.crashed = true;
                losers++;
            }

            if (rocket.pos.x > blockerPos.x && rocket.pos.x < (blockerPos.x + blockerSize.x) && rocket.pos.y < (blockerPos.y + blockerSize.y) && rocket.pos.y > blockerPos.y)
            {
                rocket.vel *= 0.0f;
                rocket.acc *= 0.0f;
                rocket.crashed = true;
                losers++;
            }

            rocket.applyForce(rocket.dna.genes[age] * fElapsedTime);
            rocket.update();
        }

        Clear(olc::BLACK);
        SetPixelMode(olc::Pixel::ALPHA);
        DrawSprite(target, sprTarget, 0.2f);
        for (auto &rocket : rockets)
        {
            if (rocket.completed)
            {
                DrawSprite(rocket.pos, sprTarget, 0.5f);
            }
            else
            {
                DrawRotatedDecal(
                    rocket.pos,
                    dRocket,
                    //0.0f,
                    atan2f(rocket.vel.y - rocket.pos.y, rocket.vel.x - rocket.pos.x) + (PI / 2),
                    {sprRocket->width / 2.0f, sprRocket->height / 2.0f});
            }
        }

        FillRect(blockerPos, blockerSize, olc::RED);
        DrawString({10, 30}, "Age: " + std::to_string(age), olc::WHITE, 2);

        if (winners > maxWinners)
        {
            maxWinners = winners;
        }
        DrawString({10, 50}, "Winners: " + std::to_string(maxWinners) + "/" + std::to_string(numberOfRockets), olc::WHITE, 2);
        DrawString({10, 70}, "Losers: " + std::to_string(losers), olc::WHITE, 2);
        DrawString({10, 90}, "Generation: " + std::to_string(generation), olc::WHITE, 2);
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
