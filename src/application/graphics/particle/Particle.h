#ifndef LAKOT_PARTICLE_H
#define LAKOT_PARTICLE_H

#include <glm/glm.hpp>

class Particle
{
private:
    glm::vec3 mPosition;
    glm::vec3 mVelocity;
    glm::vec4 mColor;

    double mLifeTime;
    double mCurrentTime;
    double mSize;

    bool mIsActive;

  public:
    virtual ~Particle();
    Particle();

    void update(double pTimeDifference);

    const glm::vec3& getPosition() const;
    const glm::vec3& getVelocity() const;
    const glm::vec4& getColor() const;

    double getLifeTime() const;
    double getSize() const;

    bool getIsActive() const;

    void setPosition(const glm::vec3& pPosition);
    void setVelocity(const glm::vec3& pVelocity);
    void setColor(const glm::vec4& pColor);

    void setLifeTime(double pLifeTime);
    void setSize(double pSize);

    void setIsActive(bool pIsActive);
};

#endif
