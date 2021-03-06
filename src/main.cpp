#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <stdint.h>
#include <stdlib.h>
#include "resultfilename.h"
#include "timer.h"
#include "test.h"

static const size_t kTrial = 10;
static const size_t kCount = 1000000;

template <typename T>
static void Verify(void(*f)(T*, size_t), const char* test, const char* fname) {
    printf("Verifying %s %s ... ", test, fname);

    void* p = malloc(kCount * sizeof(T) + 32);
    T* data = reinterpret_cast<T*>(((uintptr_t)p + 32) & ~31);
    f(data, kCount);

    // Compute mean, minimum and maximum
    double sum = 0.0;
    double minimum = data[0];
    double maximum = data[0];
    for (size_t i = 0; i < kCount; i++) {
        sum += data[i];
        if (data[i] < minimum) minimum = data[i];
        if (data[i] > maximum) maximum = data[i];
    }

    double mean = sum / kCount;

    // Compute standard deviation
    double sqDeltaSum = 0.0;
    for (size_t i = 0; i < kCount; i++) {
        double delta = data[i] - mean;
        sqDeltaSum += delta * delta;
    }
    double sd = std::sqrt(sqDeltaSum / kCount);

    // Compute skewness
    double skewnessSum = 0.0;
    for (size_t i = 0; i < kCount; i++) {
        double term = (data[i] - mean) / sd;
        skewnessSum += term * term * term;
    }
    double skewness = skewnessSum / kCount;

    // Compute kurtosis
    double kurtosisSum = 0.0;
    for (size_t i = 0; i < kCount; i++) {
        double delta = (data[i] - mean);
        kurtosisSum += (delta * delta) * (delta * delta);
    }
    double kurtosis = (kurtosisSum / kCount) / ((sd * sd) * (sd * sd)) - 3.0;

    free(p);

    if (std::abs(mean) < 0.01 && std::abs(sd - 1.0) < 0.01 && std::abs(skewness) < 0.01 && std::abs(kurtosis) < 0.01)
        printf("OK\n");
    else
        printf("Fail\n");

    printf(
        "mean     = % .6f\n"
        "SD       = % .6f\n"
        "minimum  = % .6f\n"
        "maximum  = % .6f\n"
        "skewness = % .6f\n"
        "kurtosis = % .6f\n\n",
        mean, sd, minimum, maximum, skewness, kurtosis);
}

static void VerifyAll() {
    const TestList& tests = TestManager::Instance().GetTests();
    for (TestList::const_iterator itr = tests.begin(); itr != tests.end(); ++itr) {
        if (strcmp((*itr)->fname, "null") != 0) {   // skip null
            if ((*itr)->normaldistf)
                Verify((*itr)->normaldistf, "normaldistf", (*itr)->fname);
            if ((*itr)->normaldist)
                Verify((*itr)->normaldist, "normaldist", (*itr)->fname);
        }
    }
}

template <typename T>
static void Bench(void(*f)(T*, size_t), const char* type, const char* fname, FILE* fp) {
    printf("Benchmarking     %-20s ... ", fname);

    double duration = std::numeric_limits<double>::max();
    char* p = static_cast<char*>(malloc(kCount * sizeof(T) + 32));
    T* data = reinterpret_cast<T*>(((uintptr_t)p + 32) & ~31);
    f(data, kCount);
    for (unsigned trial = 0; trial < kTrial; trial++) {
        Timer timer;
        timer.Start();

        f(data, kCount);

        timer.Stop();
        duration = std::min(duration, timer.GetElapsedMilliseconds());
    }
    free(p);

    duration *= 1e6 / kCount; // convert to nano second per operation
    fprintf(fp, "%s,%s,%f\n", type, fname, duration);

    printf("%8.3fns\n", duration);
}

static void BenchAll() {
    // Try to write to /result path, where template.php exists
    FILE *fp;
    if ((fp = fopen("../../result/template.php", "r")) != NULL) {
        fclose(fp);
        fp = fopen("../../result/" RESULT_FILENAME, "w");
    }
    else if ((fp = fopen("../result/template.php", "r")) != NULL) {
        fclose(fp);
        fp = fopen("../result/" RESULT_FILENAME, "w");
    }
    else
        fp = fopen(RESULT_FILENAME, "w");

    fprintf(fp, "Type,Function,Time(ns)\n");

    const TestList& tests = TestManager::Instance().GetTests();

    puts("normaldistf");
    for (TestList::const_iterator itr = tests.begin(); itr != tests.end(); ++itr)
        if ((*itr)->normaldistf)
            Bench((*itr)->normaldistf, "normaldistf", (*itr)->fname, fp);

    puts("\nnormaldist");
    for (TestList::const_iterator itr = tests.begin(); itr != tests.end(); ++itr)
        if ((*itr)->normaldist)
            Bench((*itr)->normaldist, "normaldist", (*itr)->fname, fp);

    fclose(fp);
}

int main() {
    // sort tests
    TestList& tests = TestManager::Instance().GetTests();
    std::sort(tests.begin(), tests.end());

    VerifyAll();
    BenchAll();
}
