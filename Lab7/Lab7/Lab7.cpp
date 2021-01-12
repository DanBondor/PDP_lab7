// Lab7.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <cstdlib>
#include <thread>
#include <atomic>

#include "mpi.h"

std::atomic<int> currProc = 0;

std::vector<int> createPolynomial(int n) {

	std::vector<int> res;
	for (int i = 0; i < n; i++) {
		res.push_back(rand() % n);
	}
	return res;
}

void prettyPrintPolynomial(std::vector<int> poly) {
	for (int i = 0; i < poly.size(); i++) {
		std::cout << poly[i];
		if (i != 0) {
			std::cout << "X^" << i;
		}
		if (i != poly.size() - 1) {
			std::cout << " + ";
		}
	}
	std::cout << "\n";
}

int computePoly(std::vector<int>& a, std::vector<int>& b, int index) {
	int val = 0;
	for (int i = 0; i <= index; i++) {
		if (i < a.size() && index - i < b.size()) {
			val += a[i] * b[index - i];
		}
	}
	return val;
}

//-----------------------------------------------------------------------------------
void slaveSimple(int processNr) {

	std::cout << "pregatesc terenu sclav 1\n";
	int size1, size2, start, end;
	std::vector<int> a, b, res;
	MPI_Status status;

	MPI_Recv(&size1, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
	a.resize(size1);
	MPI_Recv(a.data(), size1, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);

	MPI_Recv(&size2, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &status);
	b.resize(size2);
	MPI_Recv(b.data(), size2, MPI_INT, 0, 4, MPI_COMM_WORLD, &status);

	std::cout << "pregatesc terenu sclav 2\n";
	MPI_Recv(&start, 1, MPI_INT, 0, 5, MPI_COMM_WORLD, &status);
	MPI_Recv(&end, 1, MPI_INT, 0, 6, MPI_COMM_WORLD, &status);
	for (int i = 0; i < end - start; i++) {
		res.push_back(computePoly(a, b, start + i));
	}
	int size = res.size();

	std::cout << "pregatesc terenu sclav 3\n";
	MPI_Ssend(&size, 1, MPI_INT, 0, 7, MPI_COMM_WORLD);
	MPI_Ssend(res.data(), size, MPI_INT, 0, 8, MPI_COMM_WORLD);
}

std::vector<int> masterSimple(std::vector<int>& a, std::vector<int>& b, int noProcesses) {
	std::vector<int> res;
	int start, perNr, process;
	start = 0;
	perNr = (a.size() + b.size() - 1) / noProcesses;

	int size1 = a.size();
	MPI_Bcast(&size1, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(a.data(), size1, MPI_INT, 0, MPI_COMM_WORLD);


	int size2 = b.size();
	MPI_Bcast(&size2, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(b.data(), size2, MPI_INT, 0, MPI_COMM_WORLD);

	for (process = 1; process < noProcesses; process++) {
		//send start
		int startIdx = start;
		MPI_Ssend(&startIdx, 1, MPI_INT, process, 5, MPI_COMM_WORLD);
		//send end
		int endIdx = startIdx + perNr;
		MPI_Ssend(&endIdx, 1, MPI_INT, process, 6, MPI_COMM_WORLD);

		start += perNr;
	}

	res.resize(a.size() + b.size() - 1);
	for (int val = start; val < res.size(); val++) {
		res[val] = computePoly(a, b, val);
	}

	int idx = 0;
	for (process = 1; process < noProcesses; process++) {
		int size;
		std::vector<int> partialRes;
		MPI_Status status;
		MPI_Recv(&size, 1, MPI_INT, 0, 7, MPI_COMM_WORLD, &status);
		partialRes.resize(size);
		MPI_Recv(partialRes.data(), size, MPI_INT, 0, 8, MPI_COMM_WORLD, &status);
		for (auto value : partialRes) {
			res[idx] = value;
			idx++;
		}
	}
	return res;
}

std::vector<int> seqAlg(std::vector<int>& a, std::vector<int>& b) {
	std::vector<int> res;
	for (int i = 0; i < a.size() + b.size() - 1; i++) {
		res.push_back(computePoly(a, b, i));
	}
	return res;
}

std::vector<int> karatsuba(std::vector<int>& a, std::vector<int>& b) {
	if (a.size() <= 10 || b.size() <= 10) {
		std::vector<int> v = seqAlg(a, b);
		v.push_back(0);
		return v;
	}

	int size = a.size() / 2;
	std::vector<int> aL(size), aH(size), bL(size), bH(size), aLaH(size), bLbH(size);
	for (int i = 0; i < size; i++) {
		aH[i] = a[i];
		bH[i] = b[i];
		aL[i] = a[size + i];
		bL[i] = b[size + i];
		aLaH[i] = aH[i] + aL[i];
		bLbH[i] = bH[i] + bL[i];
	}
	std::vector<int> prodL, prodH, prodLH;
	prodL = karatsuba(aL, bL);
	prodH = karatsuba(aH, bH);
	prodLH = karatsuba(aLaH, bLbH);


	std::vector<int> prod;
	for (int i = 0; i < a.size(); i++) {
		prod.push_back(prodLH[i] - prodL[i] - prodH[i]);
	}

	std::vector<int> kara(a.size() * 2);
	for (int i = 0; i < a.size(); i++) {
		kara[i] += prodH[i];
		kara[i + a.size()] += prodL[i];
		kara[i + a.size() / 2] += prod[i];
	}
	return kara;
}

void slaveKaratsuba(int processNr) {
	std::cout << processNr << "\n";
	int size1, size2;
	std::vector<int> a, b;
	MPI_Status status;

	MPI_Recv(&size1, 1, MPI_INT, 0, processNr * 10 + 1, MPI_COMM_WORLD, &status);
	a.resize(size1);
	MPI_Recv(a.data(), size1, MPI_INT, 0, processNr * 10 + 2, MPI_COMM_WORLD, &status);

	MPI_Recv(&size2, 1, MPI_INT, 0, processNr * 10 + 3, MPI_COMM_WORLD, &status);
	b.resize(size2);
	MPI_Recv(b.data(), size2, MPI_INT, 0, processNr * 10 + 4, MPI_COMM_WORLD, &status);

	std::vector<int> res = karatsuba(a, b);
	int size = res.size();
	MPI_Ssend(res.data(), size, MPI_INT, 0, processNr * 10 + 5, MPI_COMM_WORLD);
}

std::vector<int> masterKaratsuba(std::vector<int>& a, std::vector<int>& b, int depth, int noProcesses) {
	if (a.size() <= 10 || b.size() <= 10) {
		std::vector<int> v = seqAlg(a, b);
		v.push_back(0);
		return v;
	}

	int size = a.size() / 2;
	std::vector<int> aL(size), aH(size), bL(size), bH(size), aLaH(size), bLbH(size);
	for (int i = 0; i < size; i++) {
		aH[i] = a[i];
		bH[i] = b[i];
		aL[i] = a[size + i];
		bL[i] = b[size + i];
		aLaH[i] = aH[i] + aL[i];
		bLbH[i] = bH[i] + bL[i];
	}
	std::vector<int> prodL, prodH, prodLH;
	int idL, idH, idLH;
	if (log(noProcesses) / log(3) == (double)depth + 1) {
		int Lsize1, Lsize2, Hsize1, Hsize2, LHsize1, LHsize2;

		currProc += 1;
		idL = currProc;

		Lsize1 = aL.size();
		MPI_Ssend(&Lsize1, 1, MPI_INT, 0, currProc * 10 + 1, MPI_COMM_WORLD);
		MPI_Ssend(aL.data(), Lsize1, MPI_INT, 0, currProc * 10 + 2, MPI_COMM_WORLD);

		Lsize2 = bL.size();
		MPI_Ssend(&Lsize2, 1, MPI_INT, 0, currProc * 10 + 3, MPI_COMM_WORLD);
		MPI_Ssend(bL.data(), Lsize2, MPI_INT, 0, currProc * 10 + 4, MPI_COMM_WORLD);


		currProc += 1;
		idH = currProc;

		Hsize1 = aH.size();
		MPI_Ssend(&Hsize1, 1, MPI_INT, 0, currProc * 10 + 1, MPI_COMM_WORLD);
		MPI_Ssend(aH.data(), Hsize1, MPI_INT, 0, currProc * 10 + 2, MPI_COMM_WORLD);

		Hsize2 = bH.size();
		MPI_Ssend(&Hsize2, 1, MPI_INT, 0, currProc * 10 + 3, MPI_COMM_WORLD);
		MPI_Ssend(bH.data(), Hsize2, MPI_INT, 0, currProc * 10 + 4, MPI_COMM_WORLD);


		currProc += 1;
		idLH = currProc;

		LHsize1 = aLaH.size();
		MPI_Ssend(&LHsize1, 1, MPI_INT, 0, currProc * 10 + 1, MPI_COMM_WORLD);
		MPI_Ssend(aLaH.data(), LHsize1, MPI_INT, 0, currProc * 10 + 2, MPI_COMM_WORLD);

		LHsize2 = bLbH.size();
		MPI_Ssend(&LHsize2, 1, MPI_INT, 0, currProc * 10 + 3, MPI_COMM_WORLD);
		MPI_Ssend(bLbH.data(), LHsize2, MPI_INT, 0, currProc * 10 + 4, MPI_COMM_WORLD);


		MPI_Status statL, statH, statLH;
		int sizeL = aL.size() + bL.size() - 1;
		prodL.resize(sizeL);
		MPI_Recv(prodL.data(), sizeL, MPI_INT, 0, currProc * 10 + 5, MPI_COMM_WORLD, &statL);

		int sizeH = aH.size() + bH.size() - 1;
		prodH.resize(sizeH);
		MPI_Recv(prodH.data(), sizeH, MPI_INT, 0, currProc * 10 + 5, MPI_COMM_WORLD, &statH);

		int sizeLH = aLaH.size() + bLbH.size() - 1;
		prodLH.resize(sizeLH);
		MPI_Recv(prodLH.data(), sizeLH, MPI_INT, 0, currProc * 10 + 5, MPI_COMM_WORLD, &statLH);
	}
	else {
		prodL = masterKaratsuba(aL, bL, depth + 1, noProcesses);
		prodH = masterKaratsuba(aH, bH, depth + 1, noProcesses);
		prodLH = masterKaratsuba(aLaH, bLbH, depth + 1, noProcesses);
	}

	std::vector<int> prod;
	for (int i = 0; i < a.size(); i++) {
		prod.push_back(prodLH[i] - prodL[i] - prodH[i]);
	}

	std::vector<int> kara(a.size() * 2);
	for (int i = 0; i < a.size(); i++) {
		kara[i] += prodH[i];
		kara[i + a.size()] += prodL[i];
		kara[i + a.size() / 2] += prod[i];
	}
	return kara;
}

//-----------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);
	int me;
	int noProcesses;
	MPI_Comm_size(MPI_COMM_WORLD, &noProcesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &me);
	unsigned n = 10;
	std::vector<int> a, b, poly;


	auto start = std::chrono::steady_clock::now();
	if (me == 0) {
		//master
		a = createPolynomial(n);
		b = createPolynomial(n);
		//poly = masterSimple(a, b, noProcesses);
		poly = masterKaratsuba(a, b, 0, noProcesses);
		poly.pop_back();
	}
	else {
		//slave

		//slaveSimple(me);
		slaveKaratsuba(me);
	}
	auto end = std::chrono::steady_clock::now();
	prettyPrintPolynomial(a);
	prettyPrintPolynomial(b);

	prettyPrintPolynomial(poly);

	std::chrono::duration<double> seconds = end - start;
	std::cout.precision(7);
	std::cout << "Polynomial in this form: " << std::fixed << seconds.count() << "\n";
	MPI_Finalize();
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
