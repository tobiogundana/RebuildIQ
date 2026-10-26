
#pragma once

#include <vector>

struct RecoveryJob {
    int id {};
    double size {};
    int replicas {};
    double risk {};
    double failureRate {};
};

std::vector<double> calculateCompletionTimes(
    const std::vector<RecoveryJob>& jobs, double bandwidth);

std::vector<RecoveryJob> shortestJobFirst(std::vector<RecoveryJob> jobs);

std::vector<RecoveryJob> riskAwareScheduling(std::vector<RecoveryJob> jobs);

double averageCompletionTime(const std::vector<double>& times);

double calculateRiskExposure(
    const std::vector<RecoveryJob>& jobs,
    const std::vector<double>& completionTimes);

double calculateDataLossProbability(
    const RecoveryJob& job, double completionTime);

double calculateExpectedDataLoss(
    const std::vector<RecoveryJob>& jobs,
    const std::vector<double>& completionTimes);

double calculateFailureProbability(
    const RecoveryJob& job, double currentTime, double bandwidth);

std::vector<RecoveryJob> failureAwareScheduling(
    std::vector<RecoveryJob> jobs, double bandwidth);

std::vector<RecoveryJob> generateWorkload(
    int count, unsigned int seed);

std::vector<double> calculateConcurrentCompletionTimes(
    const std::vector<RecoveryJob>& jobs,
    double bandwidth,
    std::size_t maxConcurrent
);

double calculateAvailableRecoveryBandwidth(
    double totalBandwidth,
    double clientDemand
);


std::vector<double> calculateParallelCompletionTimes(
    const std::vector<RecoveryJob>& jobs,
    double diskBandwidth,
    double systemBandwidth,
    std::size_t maxConcurrent
);
