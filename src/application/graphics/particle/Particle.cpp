#include "Particle.h"

Particle::~Particle()
{

}

Particle::Particle()
    : mIsActive(false)
{
    
}

void Particle::update(double pTimeDifference)
{
    if (!mIsActive)
    {
        return;
    }

    mCurrentTime += pTimeDifference;

    if (mCurrentTime > mLifeTime)
    {
        mIsActive = false;
        return;
    }

    mPosition += static_cast<float>(pTimeDifference) * mVelocity;
}

const glm::vec3& Particle::getPosition() const
{
    return mPosition;
}

const glm::vec3& Particle::getVelocity() const
{
    return mVelocity;
}

const glm::vec4& Particle::getColor() const
{
    return mColor;
}

double Particle::getLifeTime() const
{
    return mLifeTime;
}

double Particle::getSize() const
{
    return mSize;
}

bool Particle::getIsActive() const
{
    return mIsActive;
}

void Particle::setPosition(const glm::vec3& pPosition)
{
    mPosition = pPosition;
}

void Particle::setVelocity(const glm::vec3& pVelocity)
{
    mVelocity = pVelocity;
}

void Particle::setColor(const glm::vec4& pColor)
{
    mColor = pColor;
}

void Particle::setLifeTime(double pLifeTime)
{
    mLifeTime = pLifeTime;
}

void Particle::setSize(double pSize)
{
    mSize = pSize;
}

void Particle::setIsActive(bool pIsActive)
{
    mIsActive = pIsActive;
}
