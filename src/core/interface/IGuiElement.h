#ifndef IGUIELEMENT_H
#define IGUIELEMENT_H

class IGuiElement
{
public:
    virtual ~IGuiElement();
    IGuiElement();

    // TODO: change param.
    virtual void render(void* pObject) = 0;
};

#endif // IGUIELEMENT_H
