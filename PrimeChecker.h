#pragma once
class PrimeChecker
{
public:
	PrimeChecker();
	~PrimeChecker();
	bool isPrime(int number);
private:
	bool isDivisible(int number, int divisor);
};

