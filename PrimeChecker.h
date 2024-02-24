#pragma once

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