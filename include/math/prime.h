#ifndef PRIME_H
#define PRIME_H
#include <stdint.h>
#include <stdbool.h>

// Finds the next prime larger than n
uint64_t prime_next(uint64_t n);

// Returns true if the given number is prime
bool prime_test(uint64_t n, int iteration);
#endif