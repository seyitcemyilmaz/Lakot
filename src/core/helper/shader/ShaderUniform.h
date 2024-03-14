#ifndef LAKOT_SHADERUNIFORM_H
#define LAKOT_SHADERUNIFORM_H

#include <string>

class ShaderUniform
{
public:
    ShaderUniform(const std::string& pName);

    const std::string& getName() const;

    int getLocation() const;
    void setLocation(int pLocation);

private:
    std::string mName;

    int mLocation;
};

#endif
