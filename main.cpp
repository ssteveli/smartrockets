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
    olc::Sprite *sprExplosion = NULL;
    olc::Decal *dExplosion = NULL;

    struct Particle
    {
        olc::vf2d pos;
        olc::vf2d vel;
        float angle;
        float time;
        olc::Pixel col;
    };

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
    std::vector<Particle> particles;
    olc::vf2d target;
    olc::vf2d targetCenter;
    olc::vf2d blockerPos;
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

    void hit(Rocket &rocket)
    {
        if (rocket.crashed)
            return;

        rocket.crashed = true;

        for (int i = 0; i < 3; i++)
        {
            Particle p;
            p.pos = {rocket.pos.x + 0.0f, rocket.pos.y + 1.0f};
            p.angle = float(rand()) / float(RAND_MAX) * 2.0f * PI;
            float v = float(rand()) / float(RAND_MAX) * 15.0f;
            p.vel = {v * (float)cos(p.angle), v * (float)sin(p.angle)};
            p.time = 1.0f;
            p.col = olc::DARK_RED;

            particles.push_back(p);
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
        dRocket = new olc::Decal(sprRocket);

        sprExplosion = new olc::Sprite("./assets/explosion.png");
        dExplosion = new olc::Decal(sprExplosion);

        sprTarget = new olc::Sprite("./assets/star.png");
        target = {(ScreenWidth() / 2.0f) - (sprTarget->width / 2.0f), 50.0f};
        targetCenter = {target.x + (sprTarget->width / 2), target.y - (sprTarget->height / 2)};

        blockerPos = {((float)ScreenWidth() / 2) - blockerSize.x / 2, (float)ScreenHeight() * 0.60f};

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
                hit(rocket);
                losers++;
            }

            rocket.applyForce(rocket.dna.genes[age] * fElapsedTime);
            rocket.update();
        }

        for (auto &p : particles)
        {
            p.vel += olc::vf2d(0.0f, 50.0f) * fElapsedTime;
            p.pos += p.vel * fElapsedTime;
            p.angle += 5.0f * fElapsedTime;
            p.time -= fElapsedTime;
            p.col = (p.time / 3.0f) * 255;
        }

        particles.erase(std::remove_if(particles.begin(), particles.end(),
                                       [](const Particle p) {
                                           return p.time < 0.0f;
                                       }),
                        particles.end());

        Clear(olc::BLACK);
        SetPixelMode(olc::Pixel::ALPHA);
        DrawSprite(target, sprTarget, 0.2f);
        for (auto &rocket : rockets)
        {
            if (rocket.completed)
            {
                DrawSprite(rocket.pos, sprTarget, 0.5f);
            }
            else if (!rocket.crashed)
            {
                olc::vf2d h = rocket.vel.norm();
                float angle = atan2f(h.y, h.x) + 3.14159f / 2.0f;

                DrawRotatedDecal(
                    rocket.pos,
                    dRocket,
                    angle,
                    {sprRocket->width / 2.0f, sprRocket->height / 2.0f});
            }
        }

        for (auto &p : particles)
        {
            DrawCircle(p.pos, 2.0f, olc::WHITE);
            DrawRotatedDecal(p.pos, dExplosion, p.angle);
        }

        FillRect(blockerPos, blockerSize, olc::DARK_RED);
        DrawString({10, 30}, "Age: " + std::to_string(age), olc::WHITE, 2);

        if (winners > maxWinners)
        {
            maxWinners = winners;
        }
        DrawString({10, 50}, "Winners: " + std::to_string(maxWinners) + "/" + std::to_string(numberOfRockets), olc::WHITE, 2);
        DrawString({10, 70}, "Losers: " + std::to_string(losers), olc::WHITE, 2);
        DrawString({10, 90}, "Generation: " + std::to_string(generation), olc::WHITE, 2);
        SetPixelMode(olc::Pixel::NORMAL);

        return !GetKey(olc::Key::ESCAPE).bPressed;
    }
};

int main(int, char **)
{
    SmartRockets SmartRockets;
    if (SmartRockets.Construct(800, 800, 1, 1))
        SmartRockets.Start();

    return 0;
}
