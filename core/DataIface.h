#pragma once

#include "CanardIface.h"

namespace CubeFramework
{

// DataIface to CanardIface
class DataIface
{
public:
    virtual void init() = 0;
    virtual bool send(const CanardCANFrame &frame) = 0;
    virtual void update_rx() = 0;
};

} // namespace CubeFramework
