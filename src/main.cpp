
#include "recovery.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <chrono>
#include <vector>

void runExperiment(
    int jobCount,
    unsigned int seed,
    double diskBandwidth,
    double systemBandwidth,
    std::ofstream& csv
) {
    auto jobs = generateWorkload(jobCount, seed);

    std::vector<double> clientLoads = {0.0, 0.25, 0.50, 0.75};
    std::vector<std::size_t> concurrencyLevels = {1, 2, 3, 4, 8, 16};

    auto evaluate = [&](const std::string& name, auto scheduler) {
        auto start = std::chrono::steady_clock::now();

        auto scheduledJobs = scheduler();

        auto end = std::chrono::steady_clock::now();

        double executionMs =
            std::chrono::duration<double, std::milli>(end - start).count();

        for (double clientLoad : clientLoads) {
            double clientDemand = systemBandwidth * clientLoad;

            double availableBandwidth =
                calculateAvailableRecoveryBandwidth(
                    systemBandwidth,
                    clientDemand
                );

            for (std::size_t concurrency : concurrencyLevels) {
                auto completionTimes = calculateParallelCompletionTimes(
                    scheduledJobs,
                    diskBandwidth,
                    availableBandwidth,
                    concurrency
                );

                csv << jobCount << ","
                    << seed << ","
                    << diskBandwidth << ","
                    << systemBandwidth << ","
                    << clientLoad * 100.0 << ","
                    << availableBandwidth << ","
                    << name << ","
                    << concurrency << ","
                    << averageCompletionTime(completionTimes) << ","
                    << calculateRiskExposure(scheduledJobs, completionTimes) << ","
                    << calculateExpectedDataLoss(scheduledJobs, completionTimes) << ","
                    << executionMs << "\n";
            }
        }
    };

    evaluate("FIFO", [&]() {
        return jobs;
    });

    evaluate("SJF", [&]() {
        return shortestJobFirst(jobs);
    });

    evaluate("Risk-aware", [&]() {
        return riskAwareScheduling(jobs);
    });

    evaluate("Failure-aware", [&]() {
        return failureAwareScheduling(jobs, diskBandwidth);
    });
}

int main() {
    std::ofstream csv("results.csv");

    if (!csv) {
        std::cerr << "Failed to open results.csv\n";
        return 1;
    }

    csv << "job_count,seed,disk_bandwidth,system_bandwidth,"
           "client_load_percent,available_bandwidth,algorithm,"
           "concurrency,avg_completion_hours,risk_exposure,"
           "expected_data_losses,execution_ms\n";

    csv << std::setprecision(12);

    std::vector<int> jobCounts = {100, 1000, 10000};

    double diskBandwidth = 100.0;
    double systemBandwidth = 400.0;
    unsigned int seedCount = 20;

    for (int jobCount : jobCounts) {
        for (unsigned int seed = 0; seed < seedCount; ++seed) {
            runExperiment(
                jobCount,
                seed,
                diskBandwidth,
                systemBandwidth,
                csv
            );
        }

        std::cout << "Completed " << jobCount
                  << "-job experiments across "
                  << seedCount << " seeds\n";
    }

    std::cout << "Results saved to results.csv\n";

    
    return 0;
}
