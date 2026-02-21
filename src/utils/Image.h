#ifndef LAKOT_IMAGE_H
#define LAKOT_IMAGE_H

struct Image
{
    unsigned char* data;

    int width;
    int height;
    int channels;

    ~Image()
    {
        if (data)
        {
            delete [] data;
            data = nullptr;
        }
    }

    Image()
        : data(nullptr)
        , width(-1)
        , height(-1)
        , channels(-1)
    {

    }
};

#endif
