#include "core/wtc640/hungariandeadpixels.h"

#include <boost/heap/fibonacci_heap.hpp>

#include <algorithm>
#include <cassert>
#include <limits>
#include <cmath>
#include <iostream>

// reading about the the O(n^3) version at https://en.wikipedia.org/wiki/Hungarian_algorithm might be useful

namespace core
{

using std::vector;
using std::max;
using boost::heap::fibonacci_heap;


template<typename PointNeighborhoodRevealerType, typename JobToPointType, typename WorkerToPointType,
         typename WorkerAtPointRevealerType, typename CostLambdaType>
vector<int> hungarianNeighborhoodMatcher(int numberOfJobs, int numberOfWorkers, int numberOfPoints,
                                         PointNeighborhoodRevealerType pointNeighborhoodRevealer, JobToPointType jobToPoint, WorkerToPointType workerToPoint,
                                         WorkerAtPointRevealerType workerAtPointRevealer, CostLambdaType costLambda)
{
    assert(numberOfJobs < numberOfWorkers && numberOfJobs >= 0);

    struct SemiEdge
    {
        RealType lowerBoundAdjustedDelta;
        int job;
        int point;
        //auto operator<=> (const SemiEdge&) const = default; // NOT a good idea (min. heap vs max. heap)
    };
    struct SemiEdgeComparator
    {
        bool operator()(const SemiEdge & lhs, const SemiEdge & rhs) const
        {
            return lhs.lowerBoundAdjustedDelta > rhs.lowerBoundAdjustedDelta; // note that std/boost has maximum heaps, we want minimum,  i.e. switch < and >
        }
    };
    using PerimeterHeap = fibonacci_heap<SemiEdge, boost::heap::compare<SemiEdgeComparator>>;
    PerimeterHeap searchPerimeter;
    vector<vector<bool>> perimeterVisits(numberOfJobs);

    struct Edge
    {
        RealType adjustedDelta;
        int zStateNumber;   // for tight correspondence to unoptimized version (an same result)
        int job;
        int worker;
        //auto operator<=> (const Edge&) const = default; // NOT a good idea (min. heap vs max. heap)
    };
    struct EdgeComparator
    {   // note that std/boost hax maximum heaps, we want minimum, i.e. switch < and >
        bool operator()(const Edge & lhs,const Edge & rhs) const
        {
            if (lhs.adjustedDelta == rhs.adjustedDelta) // we want corespondence with unoptimized version:
            {
                if (lhs.worker == rhs.worker)
                {
                    return lhs.zStateNumber > rhs.zStateNumber;
                }
                else
                {
                    return lhs.worker > rhs.worker;
                }
            }
            else
            {
                return lhs.adjustedDelta > rhs.adjustedDelta;
            }
        }
    };
    using CandidatesHeap = fibonacci_heap<Edge, boost::heap::compare<EdgeComparator>>;
    typename CandidatesHeap::handle_type nullEdgeHandle;
    CandidatesHeap minimumCandidates;
    vector<typename CandidatesHeap::handle_type> minimumCandidatesHandles(numberOfWorkers, nullEdgeHandle);

    vector<int> jobForWorker(numberOfWorkers + 1, -1);
    vector<int> workerForJob(numberOfJobs, -1);
    vector<RealType> jobPotential(numberOfJobs), workerPotential(numberOfWorkers + 1); // workerPotential(numberOfWorkers) is a potential of an IMAGINARY worker (for some tricks)

    auto updateSearchPerimeter = [&](int job, int addedPoint)
    {
        // no worker potential (always non-positive) substracted ==> lower bound:
        RealType updatedLowerBoundAdjustedDelta = costLambda(jobToPoint(job), addedPoint) - jobPotential.at(job) +
                                                  // note "preservance of order under addition" and missing perimeter elements change after potential adjustment (by delta):
                                                  (-workerPotential.at(numberOfWorkers)); // <-- adjustment (sum of deltas), instead of lowering previous perimeter elements
        assert(!perimeterVisits.at(job).empty());
        if (!perimeterVisits.at(job).at(addedPoint))
        {
            searchPerimeter.push({updatedLowerBoundAdjustedDelta, job, addedPoint});
            perimeterVisits.at(job).at(addedPoint) = true;
        }
    };

    vector <int> previousWorkerOnAlternatingPath(numberOfWorkers + 1, -1); // we have space even for the IMAGINARY one
    vector<bool> isWorkerInZ(numberOfWorkers + 1); vector<int> ZWorkers;

    auto jobPointPairPointNeighborhoodBothHeapsAdder = [&](int examinedJob, int examinedPoint, int zStateNumber)
    {
        for (int neighborPoint : pointNeighborhoodRevealer(searchPerimeter.top().point))
        {
            updateSearchPerimeter(examinedJob, neighborPoint);
            int workerAtNeighborPoint = workerAtPointRevealer(neighborPoint); // might be -1 (no worker here)
            if (workerAtNeighborPoint > -1 && !isWorkerInZ.at(workerAtNeighborPoint)) // lazy evaluation prevents invalid index read
            {
                RealType adjustedDelta = costLambda(jobToPoint(examinedJob), neighborPoint) - jobPotential.at(examinedJob) - workerPotential.at(workerAtNeighborPoint) +
                                         (-workerPotential.at(numberOfWorkers)); // <-- adjustment (sum of deltas)
                if (minimumCandidatesHandles.at(workerAtNeighborPoint) == nullEdgeHandle)
                {
                    minimumCandidatesHandles.at(workerAtNeighborPoint) = minimumCandidates.push({adjustedDelta, zStateNumber, examinedJob, workerAtNeighborPoint});
                    previousWorkerOnAlternatingPath.at(workerAtNeighborPoint) = workerForJob.at(examinedJob);
                }
                else
                {
                    auto handleToWorkerAtNeighborPoint = minimumCandidatesHandles.at(workerAtNeighborPoint);
                    assert(handleToWorkerAtNeighborPoint != nullEdgeHandle);
                    if (adjustedDelta < (*handleToWorkerAtNeighborPoint).adjustedDelta)
                    {
                        minimumCandidates.increase(handleToWorkerAtNeighborPoint, {adjustedDelta, zStateNumber , examinedJob, workerAtNeighborPoint});
                        minimumCandidatesHandles.at(workerAtNeighborPoint) = handleToWorkerAtNeighborPoint;
                        previousWorkerOnAlternatingPath.at(workerAtNeighborPoint) = workerForJob.at(examinedJob);
                    }
                }
            }
        }
    };

    auto deltaAdjustZSet = [&](RealType delta)
    {
        for (int worker : ZWorkers)
        {
            jobPotential.at(jobForWorker.at(worker)) += delta;
            workerPotential.at(worker) -= delta;
        }
    };

    auto alternateSwap = [&] (int alternatingPathEnd)
    {
        for (int previousWorkerOnPath, currentWorkerOnPath = alternatingPathEnd; currentWorkerOnPath != numberOfWorkers; currentWorkerOnPath = previousWorkerOnPath)
        {
            previousWorkerOnPath = previousWorkerOnAlternatingPath.at(currentWorkerOnPath);
            assert(previousWorkerOnPath != -1);
            jobForWorker.at(currentWorkerOnPath) = jobForWorker.at(previousWorkerOnPath);
            workerForJob.at(jobForWorker.at(previousWorkerOnPath)) = currentWorkerOnPath;
        }
        jobForWorker.at(numberOfWorkers) = -1; // sanitization
    };

    auto correctnessCheck = [&]() -> bool
    {
        RealType potentialSum = static_cast<RealType>(0), matchingSum = static_cast<RealType>(0);
        for (int job = 0; job < numberOfJobs; job++) // check validity of potential
        {
            potentialSum += jobPotential.at(job);
            matchingSum += costLambda(jobToPoint(job), workerToPoint(workerForJob.at(job)));
            if (std::abs(costLambda(jobToPoint(job), workerToPoint(workerForJob.at(job))) - jobPotential.at(job) - workerPotential.at(workerForJob.at(job))) > EPSILON)
            {
                return false;
            }
            for (int worker = 0; worker < numberOfWorkers; worker++)
            {
                if (costLambda(jobToPoint(job), workerToPoint(worker)) + EPSILON < jobPotential.at(job) + workerPotential.at(worker))
                {
                    return false;
                }
            }
        }
        for (int worker = 0; worker < numberOfWorkers; worker++)
        {
            potentialSum += workerPotential.at(worker);
        }
        if (std::abs(potentialSum - matchingSum) > EPSILON)
        {
            return false;
        }
        return true;
    };

    // main loop:
    for (int currentMatchingExpandingJob = 0; currentMatchingExpandingJob < numberOfJobs; currentMatchingExpandingJob++)
    {
        int currentZExpandingWorker = numberOfWorkers; // set to the IMAGINARY one
        jobForWorker.at(currentZExpandingWorker) = currentMatchingExpandingJob; // the IMAGINARY worker assigned to matching-expanding job
        workerForJob.at(currentMatchingExpandingJob) = currentZExpandingWorker; // symmetrically
        std::fill(previousWorkerOnAlternatingPath.begin(), previousWorkerOnAlternatingPath.end(), -1);
        std::fill(isWorkerInZ.begin(), isWorkerInZ.end(), false); ZWorkers.clear();

        searchPerimeter.clear();
        std::fill(perimeterVisits.begin(), perimeterVisits.end(), vector<bool>());

        minimumCandidates.clear();
        minimumCandidatesHandles = vector<typename CandidatesHeap::handle_type>(numberOfWorkers, nullEdgeHandle);

        int zStateNumber = 0;
        while (jobForWorker.at(currentZExpandingWorker) != -1)
        {
            isWorkerInZ.at(currentZExpandingWorker) = true; ZWorkers.push_back(currentZExpandingWorker); // first time its the IMAGINARY one
            int currentZExpandingJob = jobForWorker.at(currentZExpandingWorker); // first iteration sets the job of IMAGINARY worker
            perimeterVisits.at(currentZExpandingJob) = vector<bool>(numberOfPoints);
            RealType delta = PSEUDO_INFINITY;
            updateSearchPerimeter(currentZExpandingJob, jobToPoint(currentZExpandingJob));
            assert(!searchPerimeter.empty());
            while (!searchPerimeter.empty() &&  // non-strict order (>=) to ensure corespondence with unoptimized version:
                   (minimumCandidates.empty() || minimumCandidates.top().adjustedDelta >= searchPerimeter.top().lowerBoundAdjustedDelta))
            {
                jobPointPairPointNeighborhoodBothHeapsAdder(searchPerimeter.top().job, searchPerimeter.top().point, zStateNumber);
                searchPerimeter.pop();
            }
            assert(!minimumCandidates.empty());
            int nextWorkerOnAlternatingPath = minimumCandidates.top().worker;
            delta = costLambda(jobToPoint(minimumCandidates.top().job), workerToPoint(nextWorkerOnAlternatingPath)) -
                    jobPotential.at(minimumCandidates.top().job) - workerPotential.at(nextWorkerOnAlternatingPath);
            minimumCandidatesHandles.at(nextWorkerOnAlternatingPath) = nullEdgeHandle; // sanitization
            minimumCandidates.pop();
            deltaAdjustZSet(delta);
            currentZExpandingWorker = nextWorkerOnAlternatingPath;

            zStateNumber++;
        }
        alternateSwap(currentZExpandingWorker);
    }
    assert(correctnessCheck());
    return workerForJob;
}

template <class T> bool ckmin(T &a, const T &b) { return b < a ? a = b, 1 : 0; }
template<typename costLambdaType>
vector<int> hungarianNeighborhoodMatcherUnoptimizedDemo(int J, int W, costLambdaType costLambda)
{
    // const int J = (int)size(C), W = (int)size(C[0]);
    assert(J <= W);
    // job[w] = job assigned to w-th worker, or -1 if no job assigned
    // note: a W-th worker was added for convenience
    vector<int> job(W + 1, -1);
    vector<RealType> ys(J), yt(W + 1);  // potentials
    // -yt[W] will equal the sum of all deltas
    vector<RealType> answers;
    const RealType inf = std::numeric_limits<RealType>::max();
    for (int j_cur = 0; j_cur < J; ++j_cur) {  // assign j_cur-th job
        int w_cur = W;
        job[w_cur] = j_cur;
        // min reduced cost over edges from Z to worker w
        vector<RealType> min_to(W + 1, inf);
        vector<int> prv(W + 1, -1);  // previous worker on alternating path
        vector<bool> in_Z(W + 1);    // whether worker is in Z
        while (job[w_cur] != -1) {   // runs at most j_cur + 1 times
            in_Z[w_cur] = true;
            const int j = job[w_cur];
            RealType delta = inf;
            int w_next;
            for (int w = 0; w < W; ++w) {
                if (!in_Z[w]) {
                    if (ckmin(min_to[w], costLambda(j, w) - ys[j] - yt[w]))
                        prv[w] = w_cur;
                    if (ckmin(delta, min_to[w])) w_next = w;
                }
            }
            // delta will always be non-negative,
            // except possibly during the first time this loop runs
            // if any entries of C[j_cur] are negative
            for (int w = 0; w <= W; ++w) {
                if (in_Z[w]) ys[job[w]] += delta, yt[w] -= delta;
                else min_to[w] -= delta;
            }
            w_cur = w_next;
        }
        // update assignments along alternating path
        for (int w; w_cur != W; w_cur = w) job[w_cur] = job[w = prv[w_cur]];
        //answers.push_back(-yt[W]);
    }
    vector<int> resultMatching(J, -1);
    for (int i = 0; i < W; i++)
    {
        if (job[i] != -1)
        {
            resultMatching.at(job[i]) = i;
        }
    }
    auto demoCorrectnessCheck = [&]() -> bool
    {
        RealType epsilon = static_cast<RealType>(0.01);
        RealType potentialSum = static_cast<RealType>(0), matchingSum = static_cast<RealType>(0);
        for (int job = 0; job < J; job++) // check validity of potential
        {
            potentialSum += ys[job];
            matchingSum += costLambda(job, resultMatching.at(job));
            if (std::abs(costLambda(job, resultMatching.at(job)) - ys[job] - yt[resultMatching.at(job)]) > epsilon)
            {
                return false;
            }
            for (int worker = 0; worker < W; worker++)
            {
                if (costLambda(job, worker) + epsilon < ys[job] + yt[worker])
                {
                    return false;
                }
            }
        }
        for (int worker = 0; worker < W; worker++)
        {
            potentialSum += yt[worker];
        }
        if (std::abs(potentialSum - matchingSum) > epsilon)
        {
            return false;
        }
        return true;
    };
    assert(demoCorrectnessCheck());
    return resultMatching;
}

vector<SimplePixel> hungarianDeadPixels(int rowCount, int columnCount, std::vector<SimplePixel> deadPixels,
                                        // difference = replacement pixel coordinate - dead pixel coordinate:
                                        const std::function<RealType(int rowDifference, int columnDifference)>& costFunction)
{
    assert(rowCount > 1 && columnCount > 1);
    auto primaryPointOfPixel = [columnCount](int row, int column) -> int // basically a pixel number (= discrete space point number of pixels first use representing point)
    {
        return row * columnCount + column;
    };
    auto pointNeighborhoodRevealer = [rowCount, columnCount, primaryPointOfPixel](int point) -> vector<int>
    {
        int numberOfPixels = rowCount * columnCount;
        point %=  numberOfPixels;
        int row = point / columnCount;
        int column = point % columnCount;

        bool leftGap = column > 0;
        bool topGap = row > 0;
        bool rightGap = column < columnCount - 1;
        bool bottomGap = row < rowCount - 1;

        vector<int> result;
        result.push_back(point); // for less chaotic behavior
        result.push_back(point + numberOfPixels);
        for (int layerShift = 0; layerShift <= numberOfPixels; layerShift += numberOfPixels)
        {   // layerShift for points of secondary pixel use:
            if (topGap && leftGap)
            {
                result.push_back(primaryPointOfPixel(row - 1,   column - 1)     + layerShift);
            }
            if (topGap)
            {
                result.push_back(primaryPointOfPixel(row - 1,   column)         + layerShift);
            }
            if (topGap && rightGap)
            {
                result.push_back(primaryPointOfPixel(row - 1,   column + 1)     + layerShift);
            }
            if (leftGap)
            {
                result.push_back(primaryPointOfPixel(row,       column - 1)     + layerShift);
            }
            if (rightGap)
            {
                result.push_back(primaryPointOfPixel(row,       column + 1)     + layerShift);
            }
            if (bottomGap && leftGap)
            {
                result.push_back(primaryPointOfPixel(row + 1,   column - 1)     + layerShift);
            }
            if (bottomGap)
            {
                result.push_back(primaryPointOfPixel(row + 1,   column)         + layerShift);
            }
            if (bottomGap && rightGap)
            {
                result.push_back(primaryPointOfPixel(row + 1,   column + 1)     + layerShift);
            }
        }
        return result;
    };

    auto jobToPoint = [&](int job) -> int
    {
        return primaryPointOfPixel(deadPixels.at(job).row, deadPixels.at(job).column);
    };

    vector<bool> deadPixelMap(rowCount * columnCount, false);
    for (int i = 0; i < deadPixels.size(); i++)
    {
        assert(deadPixelMap.at(deadPixels.at(i).row * columnCount + deadPixels.at(i).column) == false && "dead pixel duplicity");
        deadPixelMap.at(deadPixels.at(i).row * columnCount + deadPixels.at(i).column) = true;
    }

    vector<SimplePixel> livePixelPositions;
    for (int i = 0; i < rowCount * columnCount; i++)
    {
        if (!deadPixelMap.at(i))
        {
            livePixelPositions.push_back({i / columnCount, i % columnCount});
        }
    }

    auto workerToPoint = [&](int worker) -> int
    {
        bool secondary = worker >= livePixelPositions.size();
        worker %= livePixelPositions.size();
        if (secondary)
        {
            return primaryPointOfPixel(livePixelPositions.at(worker).row, livePixelPositions.at(worker).column) + rowCount * columnCount;
        }
        else
        {
            return primaryPointOfPixel(livePixelPositions.at(worker).row, livePixelPositions.at(worker).column);
        }
    };

    vector<int> livePixelMap(rowCount * columnCount, -1);
    for (int i = 0; i < livePixelPositions.size(); i++)
    {
        livePixelMap.at(livePixelPositions.at(i).row * columnCount + livePixelPositions.at(i).column) = i;
    }

    auto workerAtPointRevealer = [&](int point) -> int
    {
        bool secondary = point >= rowCount * columnCount;
        point %= rowCount * columnCount;
        if (secondary)
        {
            if (livePixelMap.at(point) != -1)
            {
                return livePixelMap.at(point) + static_cast<int>(livePixelPositions.size());
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return livePixelMap.at(point);
        }
    };

    auto costLambda = [&](int jobPoint, int endPoint) -> RealType
    {
        bool secondary = endPoint >= rowCount * columnCount;
        endPoint %= rowCount * columnCount;
        int rowDifference = endPoint / columnCount - jobPoint / columnCount;
        int columnDifference = endPoint % columnCount - jobPoint % columnCount;
        if (secondary)
        {
            return costFunction(rowDifference, columnDifference) + REUSE_DEMOTIVATION; // add just to prefer different pixels over used ones
        }
        else
        {
            return costFunction(rowDifference, columnDifference);
        }
    };

    vector<int> replacementWorkers;
    if (UNOPTIMIZED_DEMO)
    {
        replacementWorkers = hungarianNeighborhoodMatcherUnoptimizedDemo(rowCount * columnCount - static_cast<int>(livePixelPositions.size()),
                                                                         static_cast<int>(livePixelPositions.size()) * 2,
                                                                         [&](int job, int worker) -> RealType
                                                                         {
                                                                             return costLambda(jobToPoint(job), workerToPoint(worker));
                                                                         });
    }
    else
    {
        replacementWorkers = hungarianNeighborhoodMatcher(rowCount * columnCount - static_cast<int>(livePixelPositions.size()),
                                                          static_cast<int>(livePixelPositions.size()) * 2,
                                                          rowCount * columnCount * 2,
                                                          pointNeighborhoodRevealer,
                                                          jobToPoint,
                                                          workerToPoint,
                                                          workerAtPointRevealer,
                                                          costLambda);
    }

    vector<SimplePixel> resultReplacementPixels;
    for(int i = 0; i < replacementWorkers.size(); i++)
    {
        int worker = replacementWorkers.at(i) % livePixelPositions.size();
        resultReplacementPixels.push_back(livePixelPositions.at(worker));
    }
    return resultReplacementPixels;
}

} // namespace core
