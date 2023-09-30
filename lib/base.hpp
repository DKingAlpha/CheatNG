#pragma once

class IValidBoolOp
{
public:
    virtual operator bool() const { return is_valid(); }
    virtual bool is_valid() const = 0;
};
