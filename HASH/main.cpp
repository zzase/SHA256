#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include<bitset>

using namespace std;
uint64_t ChangeEndian(uint64_t x)
{
	x = ((x << 8) & 0xFF00FF00FF00FF00ULL) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
	x = ((x << 16) & 0xFFFF0000FFFF0000ULL) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
	return (x << 32) | (x >> 32);
}

void PreProcess(vector<uint8_t>& message) {
	cout << "-----------------전처리 단계(PreProcess)----------------" << "\n\n";
	cout << "Message : ";
	for (int i = 0; i < message.size(); i++) {
		cout << message[i] << "";
	}
	cout << "\n";
	cout << "Message bit :";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]) << "  "; //01100001  01100010  01100011
	}

	auto L = static_cast<uint64_t>(message.size()); // message의 크기 = a,b,c 3개의 문자의 크기 = 3byte = 24bit

	message.push_back(0b10000000); //2진수 10000000을 vector에 추가 (1하나 0 7개 총 8bit) 하여 현재 message의 크기 = 4byte = 32bit
	cout << "\n\n";
	cout << "1000000(2)추가  : ";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; //01100001  01100010  01100011  10000000
	}
	cout << "\n";

	//2. 마지막 64bit를 남겨두고 나머지를 0로 채우기위해 그 나머지의 크기(K)를 구하는 과정
	//나머지(byte) = 64byte(512bit) - (기존문자열길이(byte) + 8byte(64bit,마지막 64bit) + 1byte(8bit, 위에서 끝에 1 추가한값))
	auto K = 64 - (((L % 64) + 9) % 64);

	if (K == 64) K = 0;

	//마지막 전체 512bit중 마지막 64bit를 남겨두고 그 사이를 0으로 채움 
	for (int i = 0; i < K; ++i)
	{
		message.push_back(0);
	}

	cout << "\n\n";
	cout << "마지막 64bit 제외 0 추가 \n";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; 
	}

	cout << "\n\n";

	uint64_t bitLengthInBigEndian = ChangeEndian(L);
	auto ptr = reinterpret_cast<uint8_t*>(&bitLengthInBigEndian);

	message.insert(end(message), ptr, ptr + 8);
	cout << "마지막 64bit 추가한 최종 padding 완료\n";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; 
	}
	cout << "\n\n -------------------- 끝-----------------------" << "\n";
}

//H0 구하기 

array<uint32_t, 8> Make_H0() {

	cout << "\n-----------------해싱단계1.H0구하기----------------" << "\n\n";

	array<const double,8> pList = { 2, 3, 5, 7, 11, 13, 17, 19 }; //const = 수정불가

	array<uint32_t, 8> H;

	for (int i = 0; i < pList.size(); i++) {
		auto v = sqrt(pList[i]); //루트 
		v -= static_cast<uint32_t>(v);
		v *= pow(2, 32);

		H[i] = static_cast<uint32_t>(v);

		cout <<"H0-"<< dec <<i << " : " << hex << H[i] << " = " << bitset<32>(H[i]) << "\n\n";
	}
	cout << "\n -------------------- 끝-----------------------" << "\n";

	return H;
}

// K 구하기
array<uint32_t, 64> Make_K() {

	cout << "\n-----------------해싱단계2. K 구하기----------------" << "\n\n";

	array<const double, 64> pList = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
		31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
		73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
		127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
		179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
		233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
		283, 293, 307, 311 };

	array<uint32_t, 64> K;

	for (int i = 0; i < pList.size(); i++) {
		auto v = cbrt(pList[i]); //세제곱근 
		v -= static_cast<uint32_t>(v);
		v *= pow(2, 32);

		K[i] = static_cast<uint32_t>(v);

		cout << "K-" << dec << i << " : " << hex << K[i] << " = " << bitset<32>(K[i]) << "\n\n";
	}
	cout << "\n -------------------- 끝-----------------------" << "\n";

	return K;
}

uint32_t RotateRight(uint32_t x, uint32_t n)
{
	return (x >> n) | (x << (32 - n));
}

// σ0 (Small Sigma 0)
uint32_t SSigma_0(uint32_t x)
{
	return RotateRight(x, 7) ^ RotateRight(x, 18) ^ (x >> 3);
}

// σ1 (Small Sigma 1)
uint32_t SSigma_1(uint32_t x)
{
	return RotateRight(x, 17) ^ RotateRight(x, 19) ^ (x >> 10);
}

//W 구하기 
array<uint32_t, 64> Make_W(const uint8_t(&M)[64])
{
	cout << "\n-----------------해싱단계3. W 구하기----------------" << "\n\n";
	array<uint32_t, 64> W;

	for (int i = 0; i < 16; ++i)
	{
		W[i] = ChangeEndian(reinterpret_cast<uint32_t const&>(M[i * 4]));

		cout << "W-" << dec << i << " : " << hex << W[i] << " = " << bitset<32>(W[i]) << "\n\n";
	
	}

	for (int i = 16; i < 64; ++i)
	{
		// MEXP (Message Expansion Function)
		W[i] = SSigma_1(W[i - 2]) + W[i - 7] + SSigma_0(W[i - 15]) + W[i - 16];

		cout << "W-" << dec << i << " : " << hex << W[i] << " = " << bitset<32>(W[i]) << "\n\n";
	}

	cout << "\n -------------------- 끝-----------------------" << "\n";

	return W;
}

// Σ0 (Big Sigma 0)
uint32_t BSigma_0(uint32_t x)
{
	// 인자값을 2, 13, 22 만큼 각각 오른쪽으로 돌리고 그 세값을 비교하여 합이 홀수인 곳은 1, 0이거나 짝수인 곳은 0으로 표기한다. 
	return RotateRight(x, 2) ^ RotateRight(x, 13) ^ RotateRight(x, 22);
}

// Σ1 (Big Sigma 1)
uint32_t BSigma_1(uint32_t x)
{
	// 인자값을 6, 11, 25 만큼 각각 오른쪽으로 돌리고 그 세값을 비교하여 합이 홀수인 곳은 1, 0이거나 짝수인 곳은 0으로 표기한다. 
	return RotateRight(x, 6) ^ RotateRight(x, 11) ^ RotateRight(x, 25);
}

// 인자로 사용된 세 값을 a,b,c라 하면 a의 비트가 1이면 그 위치는 b의 비트로, 0이면 c의 비트를 선택한다.
uint32_t Choose(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (~x & z);
}

//인자로 사용된 세 값의 같은 위치 비트가 1이 많으면 1, 0이 많으면 0이 된다.
uint32_t Majority(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (x & z) ^ (y & z);
}

//Round Function
std::array<uint32_t, 8> Round(std::array<uint32_t, 8> const& H, uint32_t K, uint32_t W)
{
	std::array<uint32_t, 8> nH; // next H

	auto maj = Majority(H[0], H[1], H[2]);
	auto ch = Choose(H[4], H[5], H[6]);
	auto s = K + BSigma_1(H[4]) + ch + H[7] + W;

	nH[0] = BSigma_0(H[0]) + maj + s; // a' = (Σ0(a)  + Maj(a,b,c)) + (Σ1(e)  + Ch(e,f,g) + h + W0 + K0)
	nH[1] = H[0]; //  b' = a
	nH[2] = H[1]; //  c'=b
	nH[3] = H[2]; //  d'=c
	nH[4] = H[3] + s; // e' = (Σ1(e)  + Ch(e,f,g) + h + W0 + K0) + d 
	nH[5] = H[4]; //  f'=e
	nH[6] = H[5]; //  g'=f
	nH[7] = H[6]; //  h'=g

	return nH;
}

array<uint32_t, 8> Process(vector<uint8_t> const& message)
{
	assert(message.size() % 64 == 0);

	const auto K = Make_K();
	const auto blockCount = message.size() / 64;

	auto digest = Make_H0();

	for (int i = 0; i < blockCount; ++i)
	{
		auto W = Make_W(reinterpret_cast<const uint8_t(&)[64]>(message[i * 64]));
		auto H = digest;

		for (int r = 0; r < 64; ++r)
		{
			H = Round(H, K[r], W[r]);

			cout << "---------Round " << dec << r+1 << " ------------\n\n";
			for (int j = 0; j < 8; j++) {
				cout << "H(" << dec << r+1 << ")"<<"-"<<j+1<< " : " << hex << H[j] << "\n";
			}
			cout << "\n";
		}
		cout << "\n";
		cout << "final H : ";
		for (int i = 0; i < 8; ++i)
		{
			digest[i] += H[i];

			 cout << digest[i] <<"  ";
		}
		cout << "\n";
	}

	return digest;
}
string Hexify(std::array<uint32_t, 8> const& digest)
{
	stringstream stream;

	for (auto x : digest)
	{
		stream << std::setfill('0') << std::setw(8) << std::hex << x;
	}

	return stream.str();
}

string SHA256(vector<uint8_t> message)
{
	PreProcess(message);
	auto digest = Process(message);
	return Hexify(digest);
}



int main() {
	string msg = ".";
	vector<uint8_t>message(begin(msg), end(msg)); // {"a","b","c"} 문자 하나당 1byte 차지(uint8_t = 1byte)
	cout << "\n";
	cout << "final hash : "<< SHA256(message) << "\n";

	return 0;
}

uint32_t ChangeEndian(uint32_t x)
{
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0xFF00FF);
	return (x << 16) | (x >> 16);
}

