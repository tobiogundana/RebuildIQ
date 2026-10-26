
#include "recovery.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

bool approximatelyEqual(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

void testSequentialRecovery() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 200.0, 2, 0.02, 0.00002},
        {3, 300.0, 2, 0.03, 0.00003}
    };

    auto times = calculateCompletionTimes(jobs, 100.0);

    assert(times.size() == 3);
    assert(approximatelyEqual(times[0], 1.0));
    assert(approximatelyEqual(times[1], 3.0));
    assert(approximatelyEqual(times[2], 6.0));

    assert(approximatelyEqual(averageCompletionTime(times), 10.0 / 3.0));
}

void testShortestJobFirst() {
    std::vector<RecoveryJob> jobs = {
        {1, 300.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.02, 0.00002},
        {3, 200.0, 2, 0.03, 0.00003}
    };

    auto sorted = shortestJobFirst(jobs);

    assert(sorted.size() == 3);
    assert(sorted[0].id == 2);
    assert(sorted[1].id == 3);
    assert(sorted[2].id == 1);
}

void testRiskAwareScheduling() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.05, 0.00002},
        {3, 100.0, 2, 0.03, 0.00003}
    };

    auto sorted = riskAwareScheduling(jobs);

    assert(sorted.size() == 3);
    assert(sorted[0].id == 2);
    assert(sorted[1].id == 3);
    assert(sorted[2].id == 1);
}

void testFailureAwareScheduling() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 1, 0.01, 0.00001},
        {2, 100.0, 1, 0.01, 0.00010},
        {3, 100.0, 1, 0.01, 0.00005}
    };

    auto sorted = failureAwareScheduling(jobs, 100.0);

    assert(sorted.size() == 3);
    assert(sorted[0].id == 2);
    assert(sorted[1].id == 3);
    assert(sorted[2].id == 1);
}

void testRiskExposure() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.02, 0.00002}
    };

    std::vector<double> times = {1.0, 2.0};

    double exposure = calculateRiskExposure(jobs, times);

    assert(approximatelyEqual(exposure, 0.05));
}

void testDataLossProbability() {
    RecoveryJob job = {1, 100.0, 2, 0.01, 0.001};

    double probability = calculateDataLossProbability(job, 10.0);

    double expected = std::pow(1.0 - std::exp(-0.01), 2);

    assert(approximatelyEqual(probability, expected));
}

void testExpectedDataLoss() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 1, 0.01, 0.001},
        {2, 100.0, 2, 0.01, 0.002}
    };

    std::vector<double> times = {10.0, 20.0};

    double expected =
        calculateDataLossProbability(jobs[0], times[0]) +
        calculateDataLossProbability(jobs[1], times[1]);

    assert(approximatelyEqual(
        calculateExpectedDataLoss(jobs, times),
        expected
    ));
}

void testSharedBandwidthConcurrency() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.01, 0.00001},
        {3, 100.0, 2, 0.01, 0.00001},
        {4, 100.0, 2, 0.01, 0.00001}
    };

    auto sequential = calculateCompletionTimes(jobs, 100.0);

    auto concurrent1 = calculateConcurrentCompletionTimes(
        jobs, 100.0, 1
    );

    auto concurrent2 = calculateConcurrentCompletionTimes(
        jobs, 100.0, 2
    );

    auto concurrent4 = calculateConcurrentCompletionTimes(
        jobs, 100.0, 4
    );

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        assert(approximatelyEqual(
            sequential[i],
            concurrent1[i]
        ));
    }

    assert(approximatelyEqual(concurrent2[0], 2.0));
    assert(approximatelyEqual(concurrent2[1], 2.0));
    assert(approximatelyEqual(concurrent2[2], 4.0));
    assert(approximatelyEqual(concurrent2[3], 4.0));

    for (double time : concurrent4) {
        assert(approximatelyEqual(time, 4.0));
    }
}

void testClientIOContention() {
    assert(approximatelyEqual(
        calculateAvailableRecoveryBandwidth(400.0, 0.0),
        400.0
    ));

    assert(approximatelyEqual(
        calculateAvailableRecoveryBandwidth(400.0, 100.0),
        300.0
    ));

    assert(approximatelyEqual(
        calculateAvailableRecoveryBandwidth(400.0, 200.0),
        200.0
    ));

    assert(approximatelyEqual(
        calculateAvailableRecoveryBandwidth(400.0, 300.0),
        100.0
    ));

    bool threw = false;

    try {
        calculateAvailableRecoveryBandwidth(400.0, 400.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);

    threw = false;

    try {
        calculateAvailableRecoveryBandwidth(400.0, -1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
}

void testParallelRecovery() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.01, 0.00001},
        {3, 100.0, 2, 0.01, 0.00001},
        {4, 100.0, 2, 0.01, 0.00001}
    };

    auto sequential = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 1
    );

    auto concurrent2 = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 2
    );

    auto concurrent3 = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 3
    );

    auto concurrent4 = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 4
    );

    auto concurrent8 = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 8
    );

    assert(approximatelyEqual(sequential[0], 1.0));
    assert(approximatelyEqual(sequential[1], 2.0));
    assert(approximatelyEqual(sequential[2], 3.0));
    assert(approximatelyEqual(sequential[3], 4.0));

    assert(approximatelyEqual(concurrent2[0], 1.0));
    assert(approximatelyEqual(concurrent2[1], 1.0));
    assert(approximatelyEqual(concurrent2[2], 2.0));
    assert(approximatelyEqual(concurrent2[3], 2.0));

    assert(approximatelyEqual(concurrent3[0], 1.0));
    assert(approximatelyEqual(concurrent3[1], 1.0));
    assert(approximatelyEqual(concurrent3[2], 1.0));
    assert(approximatelyEqual(concurrent3[3], 2.0));

    for (double time : concurrent4) {
        assert(approximatelyEqual(time, 1.0));
    }

    for (double time : concurrent8) {
        assert(approximatelyEqual(time, 1.0));
    }

    assert(
        averageCompletionTime(concurrent4) <
        averageCompletionTime(sequential)
    );

    assert(
        calculateRiskExposure(jobs, concurrent4) <
        calculateRiskExposure(jobs, sequential)
    );

    assert(
        calculateExpectedDataLoss(jobs, concurrent4) <
        calculateExpectedDataLoss(jobs, sequential)
    );
}

void testParallelBandwidthLimit() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.01, 0.00001},
        {3, 100.0, 2, 0.01, 0.00001},
        {4, 100.0, 2, 0.01, 0.00001}
    };

    auto unlimited = calculateParallelCompletionTimes(
        jobs, 100.0, 400.0, 4
    );

    auto limited = calculateParallelCompletionTimes(
        jobs, 100.0, 200.0, 4
    );

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        assert(approximatelyEqual(unlimited[i], 1.0));
        assert(approximatelyEqual(limited[i], 2.0));
    }
}

void testParallelUnequalJobSizes() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 200.0, 2, 0.01, 0.00001},
        {3, 300.0, 2, 0.01, 0.00001}
    };

    auto times = calculateParallelCompletionTimes(
        jobs, 100.0, 300.0, 2
    );

    assert(approximatelyEqual(times[0], 1.0));
    assert(approximatelyEqual(times[1], 2.0));
    assert(approximatelyEqual(times[2], 4.0));
}

void testParallelClientContention() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001},
        {2, 100.0, 2, 0.01, 0.00001},
        {3, 100.0, 2, 0.01, 0.00001},
        {4, 100.0, 2, 0.01, 0.00001}
    };

    double diskBandwidth = 100.0;
    double systemBandwidth = 400.0;

    double noContention = calculateAvailableRecoveryBandwidth(
        systemBandwidth, 0.0
    );

    double halfContention = calculateAvailableRecoveryBandwidth(
        systemBandwidth, 200.0
    );

    auto normal = calculateParallelCompletionTimes(
        jobs, diskBandwidth, noContention, 4
    );

    auto contended = calculateParallelCompletionTimes(
        jobs, diskBandwidth, halfContention, 4
    );

    assert(approximatelyEqual(
        averageCompletionTime(normal),
        1.0
    ));

    assert(approximatelyEqual(
        averageCompletionTime(contended),
        2.0
    ));

    assert(
        calculateRiskExposure(jobs, contended) >
        calculateRiskExposure(jobs, normal)
    );

    assert(
        calculateExpectedDataLoss(jobs, contended) >
        calculateExpectedDataLoss(jobs, normal)
    );
}

void testParallelValidation() {
    std::vector<RecoveryJob> jobs = {
        {1, 100.0, 2, 0.01, 0.00001}
    };

    bool threw = false;

    try {
        calculateParallelCompletionTimes(jobs, 0.0, 400.0, 4);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);

    threw = false;

    try {
        calculateParallelCompletionTimes(jobs, 100.0, 0.0, 4);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);

    threw = false;

    try {
        calculateParallelCompletionTimes(jobs, 100.0, 400.0, 0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
}

void testGeneratedWorkload() {
    auto jobs = generateWorkload(100, 42);

    assert(jobs.size() == 100);

    for (const auto& job : jobs) {
        assert(job.size > 0.0);
        assert(job.replicas >= 1);
        assert(job.risk >= 0.0);
        assert(job.failureRate >= 0.0);
    }

    auto repeated = generateWorkload(100, 42);

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        assert(jobs[i].id == repeated[i].id);
        assert(approximatelyEqual(jobs[i].size, repeated[i].size));
        assert(jobs[i].replicas == repeated[i].replicas);
        assert(approximatelyEqual(jobs[i].risk, repeated[i].risk));
        assert(approximatelyEqual(
            jobs[i].failureRate,
            repeated[i].failureRate
        ));
    }
}

int main() {
    testSequentialRecovery();
    testShortestJobFirst();
    testRiskAwareScheduling();
    testFailureAwareScheduling();
    testRiskExposure();
    testDataLossProbability();
    testExpectedDataLoss();
    testSharedBandwidthConcurrency();
    testClientIOContention();
    testParallelRecovery();
    testParallelBandwidthLimit();
    testParallelUnequalJobSizes();
    testParallelClientContention();
    testParallelValidation();
    testGeneratedWorkload();

    std::cout << "All RebuildIQ tests passed.\n";

    return 0;
}
