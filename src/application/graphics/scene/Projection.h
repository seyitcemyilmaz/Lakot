#ifndef PROJECTION_H
#define PROJECTION_H

#include <glm/glm.hpp>

#define LAKOT_DEFAULT_FAR_PLANE_DISTANCE            1000000.0
#define LAKOT_DEFAULT_NEAR_PLANE_DISTANCE           0.30

#define LAKOT_DEFAULT_ZOOM                          45.0

#define LAKOT_MIN_ZOOM                              1.0
#define LAKOT_MAX_ZOOM                              45.0

class Projection
{
public:
    virtual ~Projection();
    Projection();

    void onResize(double pX, double pY, double pWidth, double pHeight);

    const glm::mat4& getProjectionMatrix() const;

    double getX() const;
    double getY() const;

    double getWidth() const;
    double getHeight() const;

    double getFarPlaneDistance() const;
    double getNearPlaneDistance() const;

    double getZoom() const;

    void setX(double pX);
    void setY(double pY);

    void setWidth(double pWidth);
    void setHeight(double pHeight);

    void setFarPlaneDistance(double pFarPlaneDistance);
    void setNearPlaneDistance(double pNearPlaneDistance);

    void setZoom(double pZoom);

    void changeZoom(double pAmount);

private:
    double mX;
    double mY;

    double mWidth;
    double mHeight;

    double mFarPlaneDistance;
    double mNearPlaneDistance;

    double mZoom;

    glm::mat4 mProjectionMatrix;

    void onProjectionChanged();
};

#endif // PROJECTION_H
