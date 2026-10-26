#include "recovery.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <random>
#include <cmath>
#include <limits>
#include <vector>

std::vector<double> calculateCompletionTimes (const std::vector<RecoveryJob>& jobs, double bandwidth) {
    std::vector<double> results;
    double elapsedTime = 0.0;

    for (const RecoveryJob& job : jobs) {
        double time = job.size / bandwidth;
        elapsedTime += time;
        results.push_back(elapsedTime);
    }

    return results;
}

std::vector<RecoveryJob> shortestJobFirst (std::vector<RecoveryJob> jobs) {
    std::sort(jobs.begin(), jobs.end(), 
        [](const RecoveryJob& a, const RecoveryJob& b) {
            if (a.size == b.size) {
                return a.id < b.id;
            }
            return a.size < b.size;
        });
    
    return jobs;
}

std::vector<RecoveryJob> riskAwareScheduling (std::vector<RecoveryJob> jobs) {
    std::sort(jobs.begin(), jobs.end(),
        [](const RecoveryJob& a, const RecoveryJob& b) {
            double ratioA = a.risk / a.size;
            double ratioB = b.risk / b.size;

            if (ratioA == ratioB) {
                return a.id < b.id;
            }

            return ratioA > ratioB;

        });
    
    return jobs;
}

double averageCompletionTime(const std::vector<double>& times) {
    if (times.empty()) {
        return 0.0;
    }

    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double time = sum / times.size();
    
    return time;
};

double calculateRiskExposure(const std::vector<RecoveryJob>& jobs, const std::vector<double>& completionTimes) {
    if (jobs.size() != completionTimes.size()) {
        throw std::invalid_argument("Jobs and completion times must have equal lengths");;
    }

    double total = 0.0;

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        total += jobs[i].risk * completionTimes[i];
    }

    return total;

}

double calculateDataLossProbability(const RecoveryJob& job, double completionTime) {
    double p = std::pow((1 - std::exp(-job.failureRate * completionTime)), job.replicas);
    return p;
}

double calculateExpectedDataLoss(const std::vector <RecoveryJob>& jobs, const std::vector <double>& completionTimes ){
    if (jobs.size() != completionTimes.size()) {
        throw std::invalid_argument("Mismatched vecor sizes");
    }
    double total = 0.0;

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        auto p = calculateDataLossProbability(jobs[i], completionTimes[i]);
        total += p;
    }
    return total;
}

double calculateFailureProbability (const RecoveryJob& job, double currentTime, double bandwidth) {
    double duration = job.size / bandwidth;
    auto p1 = calculateDataLossProbability(job, currentTime + duration);
    auto p2 = calculateDataLossProbability(job, currentTime + 1 + duration);

    return (p2 - p1) / duration;
}

std::vector <double> calculateConcurrentCompletionTimes(const std::vector <RecoveryJob>& jobs, double bandwidth, std::size_t maxConcurrent) {
    if (!std::isfinite(bandwidth) || bandwidth <= 0.0 || maxConcurrent == 0) {
    throw std::invalid_argument("Invalid bandwidth or concurrency limit");
    }

    for (const RecoveryJob& job : jobs) {
        if (!std::isfinite(job.size) || job.size <= 0.0) {
            throw std::invalid_argument("Recovery job size must be positive and finite");
        }
    }
    double time = 0.0;

    std::vector <double> remainingSizes;
    std::vector <double> completionTimes(jobs.size(), 0.0);

    std::vector <std::size_t> activeJobs;

    std::size_t nextJob = 0;

    for (RecoveryJob job : jobs) {
        remainingSizes.push_back(job.size);
    }

    for (std::size_t i = 0; i < std::min(jobs.size(), maxConcurrent); ++i) {
        activeJobs.push_back(i);
        ++nextJob;
    }

    while (!activeJobs.empty()) {
        double sharedBandwidth = bandwidth / activeJobs.size();
        double nextCompletionTime = std::numeric_limits<double>::infinity();

        for (std::size_t i : activeJobs) {
            double timeToFinish = remainingSizes[i] / sharedBandwidth;
            nextCompletionTime = std::min(nextCompletionTime, timeToFinish);
        }

        time += nextCompletionTime;

        for (std::size_t i : activeJobs) {
            remainingSizes[i] -= nextCompletionTime * sharedBandwidth;
            if (remainingSizes[i] <= 1e-9) {
                completionTimes[i] = time;
            }
        }

        std::erase_if(activeJobs, [&](std::size_t i) {
            return remainingSizes[i] <= 1e-9;
        });

        while (activeJobs.size() < maxConcurrent && nextJob < jobs.size()) {
            activeJobs.push_back(nextJob);
            ++nextJob;
        }
    }

    return completionTimes;

}

std::vector<RecoveryJob> failureAwareScheduling (std::vector<RecoveryJob> jobs, double bandwidth) {
    double currentTime = 0.0;

    std::vector<RecoveryJob> schedule;
    schedule.reserve(jobs.size());

    while(!jobs.empty()) {
        std::size_t bestIndex = 0;

        double bestPriority = calculateFailureProbability(jobs[0], currentTime, bandwidth);

        for(std::size_t i = 1; i < jobs.size(); ++i) {
            auto priority = calculateFailureProbability(jobs[i], currentTime, bandwidth);

            if (priority > bestPriority) {
                bestPriority = priority;
                bestIndex = i;
            }
        }

        schedule.push_back(jobs[bestIndex]);
        currentTime += jobs[bestIndex].size / bandwidth;
        jobs.erase(jobs.begin() + bestIndex);
    }

    return schedule;
}

double calculateAvailableRecoveryBandwidth(double totalBandwidth, double clientDemand) {
    if (!std::isfinite(totalBandwidth) || totalBandwidth <= 0.0) {
    throw std::invalid_argument("Invalid total bandwidth");
    }

    if (!std::isfinite(clientDemand) || clientDemand < 0.0) {
    throw std::invalid_argument("Invalid client demand");
    }

    if (clientDemand >= totalBandwidth) {
    throw std::invalid_argument("client demand greater than or equal to total bandwidth");
    }

    return totalBandwidth - clientDemand;

}

std::vector <RecoveryJob> generateWorkload (int count, unsigned int seed) {
    std::mt19937 rng(seed);

    std::uniform_real_distribution<double> sizeDist(10.0, 1000.0);
    std::uniform_int_distribution<int> replicaDist(1, 3);
    std::uniform_real_distribution<double> riskDist(0.001, 0.05);
    std::uniform_real_distribution<double> annualFailureRateDist(0.005, 0.05);

    std::vector<RecoveryJob> result;
    result.reserve(count);

    for (int i = 1; i <= count; ++i) {
        double size = sizeDist(rng);
        int replicas = replicaDist(rng);
        double risk = riskDist(rng);
        double failureRate = -std::log(1.0 - annualFailureRateDist(rng)) / 8760.0;

        result.push_back({i, size, replicas, risk, failureRate});
    }

    return result;
    
}


std::vector<double> calculateParallelCompletionTimes(
    const std::vector<RecoveryJob>& jobs,
    double diskBandwidth,
    double systemBandwidth,
    std::size_t maxConcurrent
) {
    if (!std::isfinite(diskBandwidth) || diskBandwidth <= 0.0 ||
        !std::isfinite(systemBandwidth) || systemBandwidth <= 0.0 ||
        maxConcurrent == 0) {
        throw std::invalid_argument("Invalid recovery capacity");
    }

    std::vector<double> completionTimes(jobs.size(), 0.0);

    if (jobs.empty()) {
        return completionTimes;
    }

    struct ActiveJob {
        std::size_t index;
        double remaining;
    };

    std::vector<ActiveJob> active;
    active.reserve(std::min(maxConcurrent, jobs.size()));

    std::size_t nextJob = 0;
    double currentTime = 0.0;

    auto admitJobs = [&]() {
        while (active.size() < maxConcurrent && nextJob < jobs.size()) {
            if (!std::isfinite(jobs[nextJob].size) ||
                jobs[nextJob].size < 0.0) {
                throw std::invalid_argument("Invalid job size");
            }

            if (jobs[nextJob].size == 0.0) {
                completionTimes[nextJob] = currentTime;
            } else {
                active.push_back({nextJob, jobs[nextJob].size});
            }

            ++nextJob;
        }
    };

    admitJobs();

    while (!active.empty()) {
        double totalCapacity = std::min(
            static_cast<double>(active.size()) * diskBandwidth,
            systemBandwidth
        );

        double bandwidthPerJob =
            totalCapacity / static_cast<double>(active.size());

        double nextEvent = std::numeric_limits<double>::infinity();

        for (const auto& job : active) {
            nextEvent = std::min(
                nextEvent,
                job.remaining / bandwidthPerJob
            );
        }

        currentTime += nextEvent;

        for (auto& job : active) {
            job.remaining = std::max(
                0.0,
                job.remaining - bandwidthPerJob * nextEvent
            );
        }

        auto it = active.begin();

        while (it != active.end()) {
            if (it->remaining <= 1e-9) {
                completionTimes[it->index] = currentTime;
                it = active.erase(it);
            } else {
                ++it;
            }
        }

        admitJobs();
    }

    return completionTimes;
}
