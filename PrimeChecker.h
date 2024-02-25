#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include "Structures.h"

/**
 * This function checks if a number is prime.
 * @param n The number to check.
 * @return true if the number is prime, false otherwise.
 */
static inline bool isPrime(int n) {
	if (n <= 1)
		return false;
	if (n <= 3)
		return true;

	if (n % 2 == 0 || n % 3 == 0)
		return false;

	for (int i = 5; i * i <= n; i += 6)
	{
		if (n % i == 0 || n % (i + 2) == 0)
			return false;
	}

	return true;
}

/**
 * Splits the range into n_threads.
 */
static inline int getSizePerThread(int start, int end, int n_threads) {
	return (end - start) / n_threads;
}

/**
 * Threaded prime checker.
 * @param r The range of numbers to check.
 * @param primes The vector to store the prime numbers.
 * @param mtx The mutex to lock the vector.
 */
static inline void primeChecker(range r, std::vector<int>& primes, std::mutex& mtx) {
	for (int i = r.first; i <= r.second; i++) {
		if (isPrime(i)) {
			std::lock_guard<std::mutex> lock(mtx);
			primes.push_back(i);
		}
	}
}

/**
 * Threaded prime checker. Hex version.
 * @param r The range of numbers to check.
 * @param primes The string to store the prime numbers in hex.
 * @param mtx The mutex to lock the string.
 */
static inline void primeCheckerHex(range r, std::string& primes, std::mutex& mtx) {
	for (int i = r.first; i <= r.second; i++) {
		if (isPrime(i)) {
			std::stringstream stream;
			stream << std::hex << i;
			std::lock_guard<std::mutex> lock(mtx);
			primes += stream.str() + " ";
		}
	}
}

static inline std::vector<int> getPrimes(range r) {
	std::vector<int> primes;
	for (int i = r.first; i <= r.second; i++) {
		if (isPrime(i))
			primes.push_back(i);
	}
	return primes;
}

static inline std::string getPrimesHex(range r) {
	std::string primes;
	for (int i = r.first; i <= r.second; i++) {
		if (isPrime(i)) {
			// Convert to hex
			std::stringstream stream;
			stream << std::hex << i;
			primes += stream.str() + " ";
		}
	}
	return primes;
}

/**
 * Converts a string of hex numbers separated with space
 * to a vector of integers.
 * @param primes The string of hex numbers.
 * @return The vector of integers.
 */
static inline std::vector<int> convertHexPrimes(std::string primes) {
	std::vector<int> converted;
	std::stringstream stream(primes);
	std::string hex;

	// Convert each hex number to integer and moving
	// to the next hex number separated by space
	while (stream >> hex) {
		int n;
		std::stringstream(hex) >> std::hex >> n;
		converted.push_back(n);
	}

	return converted;
}