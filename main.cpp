#include <iostream>
#include <random>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine/olcPixelGameEngine.h"

constexpr float PI = 3.14159;
constexpr float FLOAT_MIN = -8.0f;
constexpr float FLOAT_MAX = 8.0f;

std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<float> distr(FLOAT_MIN, FLOAT_MAX);

uint8_t numberOfRockets = 50;
uint16_t lifespan = 400;
uint16_t age = 0;
uint16_t generation = 0;

float maxForce = -8.0f;
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
    olc::vf2d target;
    olc::vi2d blockerPos = {250, 400};
    olc::vi2d blockerSize = {300, 10};

    void scoreHealth()
    {
        for (auto &rocket : rockets)
        {
            float d = dist(rocket.pos, target);
            if (rocket.completed)
            {
                rocket.health *= 10;
            }
            else if (rocket.crashed)
            {
                rocket.health /= 10;
            }
            else
            {
                rocket.health = 1 / dist(rocket.pos, target);
            }
        }
    }

    void repopulate()
    {
        std::vector<Rocket> genenicPool;

        float maxHealth = 0.0f;
        for (auto &rocket : rockets)
        {
            std::cout << rocket.health << std::endl;
            if (rocket.health > maxHealth)
            {
                maxHealth = rocket.health;
            }
        }

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

        dRocket = new olc::Decal(sprRocket);

        for (int i = 0; i < numberOfRockets; i++)
        {
            Rocket rocket;
            rocket.pos = {ScreenWidth() / 2.0f, ScreenHeight() - 50.0f};
            rocket.vel = {0.0f, 0.0f};
            rocket.acc = {0.0f, 0.0f};
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
            repopulate();

            age = 0;
            generation++;
        }

        scoreHealth();
        uint16_t winners = 0;
        uint16_t losers = 0;

        for (auto &rocket : rockets)
        {
            if (dist(rocket.pos, target) < 30.0f)
            {
                rocket.vel *= 0.0f;
                rocket.acc *= 0.0f;
                rocket.completed = true;
                rocket.crashed = false;
                winners++;
                std::cout << "WIN!" << std::endl;
            }

            if (rocket.pos.x < 0 || rocket.pos.x > ScreenWidth())
            {
                rocket.vel *= 0.0f;
                rocket.acc *= 0.0f;
                rocket.crashed = true;
                losers++;
            }

            if (rocket.pos.y < 0 || rocket.pos.y > ScreenHeight())
            {
                rocket.vel *= 0.0f;
                rocket.acc *= 0.0f;
                rocket.crashed = true;
                losers++;
            }

            if (rocket.pos.x > blockerPos.x && rocket.pos.x < blockerPos.x + blockerSize.x && rocket.pos.y < blockerPos.y + blockerSize.y)
            {
                rocket.vel *= 0.0f;
                rocket.acc *= 0.0f;
                rocket.crashed = true;
                losers++;
            }

            if (!rocket.completed && !rocket.crashed)
            {
                rocket.applyForce(rocket.dna.genes[age] * fElapsedTime);
                rocket.update();
            }
        }

        Clear(olc::BLACK);
        SetPixelMode(olc::Pixel::ALPHA);
        DrawSprite(target, sprTarget, 0.2f);
        for (auto &rocket : rockets)
        {
            if (rocket.completed)
            {
                DrawSprite(rocket.pos, sprTarget, 0.005f);
            }
            else
            {
                olc::vf2d h = rocket.pos + rocket.vel;
                h = h.norm();

                DrawRotatedDecal(
                    rocket.pos,
                    dRocket,
                    atan2f(h.y, h.x),
                    {sprRocket->width / 2.0f, sprRocket->height / 2.0f});
            }
        }

        FillRect(blockerPos, blockerSize, olc::RED);
        DrawString({10, 30}, "Age: " + std::to_string(age), olc::WHITE, 2);
        DrawString({10, 50}, "Winners: " + std::to_string(winners), olc::WHITE, 2);
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
