#ifndef HUNGARIANDEADPIXELS_H
#define HUNGARIANDEADPIXELS_H

#include <limits>
#include <functional>
#include <vector>


namespace core
{
static constexpr bool UNOPTIMIZED_DEMO = false;

using RealType = long;
static constexpr RealType EPSILON = 0;                      // useful when RealType is floating point (then can be 0.01 for example), 0 for fixed point
static constexpr RealType BASE_COST = 10;                   // useful when RealType is fixed point (= scaling factor), should be 1 for floating point RealType
static constexpr RealType PREVIOUS_IMAGE_PENALTY = 20;      // double of the base cost might work
static constexpr RealType REUSE_DEMOTIVATION = 1;           // second use of pixel penalty (small number, but at least order of magnitude higher than EPSILON)
static constexpr RealType PSEUDO_INFINITY = std::numeric_limits<RealType>::max();

struct SimplePixel
{
    int row;
    int column;
    auto operator<=> (const SimplePixel&) const = default;
};

std::vector<SimplePixel> hungarianDeadPixels(int width, int height, std::vector<SimplePixel> deadPixels, const std::function<RealType(int, int)>& costFunction);

static std::vector<SimplePixel> hungarianDeadPixelsInstance(int rowCount, int columnCount, std::vector<SimplePixel> deadPixels)
{
    auto costFunction = [](int rowDifference, int columnDifference) -> RealType
    {
        RealType cost = static_cast<RealType>(0);
        cost += static_cast<RealType>(rowDifference * rowDifference + columnDifference * columnDifference) * BASE_COST;
        if(rowDifference > 0 || (rowDifference == 0 && columnDifference > 0))
        {
            cost += PREVIOUS_IMAGE_PENALTY;
        }
        return cost;
    };
    return hungarianDeadPixels(rowCount, columnCount, deadPixels, costFunction);
}

} // namespace core

#endif // HUNGARIANDEADPIXELS_H
