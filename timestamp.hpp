#pragma once
#include <iostream>
#include <string>
#include <cstdint>

/**
 * timestamp
 */
class Timestamp {
public:
    Timestamp();

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};