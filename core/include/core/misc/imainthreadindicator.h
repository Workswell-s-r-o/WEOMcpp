#ifndef IMAINTHREADINDICATOR_H
#define IMAINTHREADINDICATOR_H

namespace core
{

class IMainThreadIndicator
{
public:
    [[nodiscard]] virtual bool isInGuiThread() = 0;
    virtual ~IMainThreadIndicator(){};
};

} //namespace core
#endif // IMAINTHREADINDICATOR_H
