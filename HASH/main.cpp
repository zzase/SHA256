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
	cout << "-----------------��ó�� �ܰ�(PreProcess)----------------" << "\n\n";
	cout << "Message : ";
	for (int i = 0; i < message.size(); i++) {
		cout << message[i] << "";
	}
	cout << "\n";
	cout << "Message bit :";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]) << "  "; //01100001  01100010  01100011
	}

	auto L = static_cast<uint64_t>(message.size()); // message�� ũ�� = a,b,c 3���� ������ ũ�� = 3byte = 24bit

	message.push_back(0b10000000); //2���� 10000000�� vector�� �߰� (1�ϳ� 0 7�� �� 8bit) �Ͽ� ���� message�� ũ�� = 4byte = 32bit
	cout << "\n\n";
	cout << "1000000(2)�߰�  : ";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; //01100001  01100010  01100011  10000000
	}
	cout << "\n";

	//2. ������ 64bit�� ���ܵΰ� �������� 0�� ä������� �� �������� ũ��(K)�� ���ϴ� ����
	//������(byte) = 64byte(512bit) - (�������ڿ�����(byte) + 8byte(64bit,������ 64bit) + 1byte(8bit, ������ ���� 1 �߰��Ѱ�))
	auto K = 64 - (((L % 64) + 9) % 64);

	if (K == 64) K = 0;

	//������ ��ü 512bit�� ������ 64bit�� ���ܵΰ� �� ���̸� 0���� ä�� 
	for (int i = 0; i < K; ++i)
	{
		message.push_back(0);
	}

	cout << "\n\n";
	cout << "������ 64bit ���� 0 �߰� \n";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; 
	}

	cout << "\n\n";

	uint64_t bitLengthInBigEndian = ChangeEndian(L);
	auto ptr = reinterpret_cast<uint8_t*>(&bitLengthInBigEndian);

	message.insert(end(message), ptr, ptr + 8);
	cout << "������ 64bit �߰��� ���� padding �Ϸ�\n";
	for (int i = 0; i < message.size(); i++) {
		cout << bitset<8>(message[i]).to_string() << "  "; 
	}
	cout << "\n\n -------------------- ��-----------------------" << "\n";
}

//H0 ���ϱ� 

array<uint32_t, 8> Make_H0() {

	cout << "\n-----------------�ؽ̴ܰ�1.H0���ϱ�----------------" << "\n\n";

	array<const double,8> pList = { 2, 3, 5, 7, 11, 13, 17, 19 }; //const = �����Ұ�

	array<uint32_t, 8> H;

	for (int i = 0; i < pList.size(); i++) {
		auto v = sqrt(pList[i]); //��Ʈ 
		v -= static_cast<uint32_t>(v);
		v *= pow(2, 32);

		H[i] = static_cast<uint32_t>(v);

		cout <<"H0-"<< dec <<i << " : " << hex << H[i] << " = " << bitset<32>(H[i]) << "\n\n";
	}
	cout << "\n -------------------- ��-----------------------" << "\n";

	return H;
}

// K ���ϱ�
array<uint32_t, 64> Make_K() {

	cout << "\n-----------------�ؽ̴ܰ�2. K ���ϱ�----------------" << "\n\n";

	array<const double, 64> pList = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
		31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
		73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
		127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
		179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
		233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
		283, 293, 307, 311 };

	array<uint32_t, 64> K;

	for (int i = 0; i < pList.size(); i++) {
		auto v = cbrt(pList[i]); //�������� 
		v -= static_cast<uint32_t>(v);
		v *= pow(2, 32);

		K[i] = static_cast<uint32_t>(v);

		cout << "K-" << dec << i << " : " << hex << K[i] << " = " << bitset<32>(K[i]) << "\n\n";
	}
	cout << "\n -------------------- ��-----------------------" << "\n";

	return K;
}

uint32_t RotateRight(uint32_t x, uint32_t n)
{
	return (x >> n) | (x << (32 - n));
}

// ��0 (Small Sigma 0)
uint32_t SSigma_0(uint32_t x)
{
	return RotateRight(x, 7) ^ RotateRight(x, 18) ^ (x >> 3);
}

// ��1 (Small Sigma 1)
uint32_t SSigma_1(uint32_t x)
{
	return RotateRight(x, 17) ^ RotateRight(x, 19) ^ (x >> 10);
}

//W ���ϱ� 
array<uint32_t, 64> Make_W(const uint8_t(&M)[64])
{
	cout << "\n-----------------�ؽ̴ܰ�3. W ���ϱ�----------------" << "\n\n";
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

	cout << "\n -------------------- ��-----------------------" << "\n";

	return W;
}

// ��0 (Big Sigma 0)
uint32_t BSigma_0(uint32_t x)
{
	// ���ڰ��� 2, 13, 22 ��ŭ ���� ���������� ������ �� ������ ���Ͽ� ���� Ȧ���� ���� 1, 0�̰ų� ¦���� ���� 0���� ǥ���Ѵ�. 
	return RotateRight(x, 2) ^ RotateRight(x, 13) ^ RotateRight(x, 22);
}

// ��1 (Big Sigma 1)
uint32_t BSigma_1(uint32_t x)
{
	// ���ڰ��� 6, 11, 25 ��ŭ ���� ���������� ������ �� ������ ���Ͽ� ���� Ȧ���� ���� 1, 0�̰ų� ¦���� ���� 0���� ǥ���Ѵ�. 
	return RotateRight(x, 6) ^ RotateRight(x, 11) ^ RotateRight(x, 25);
}

// ���ڷ� ���� �� ���� a,b,c�� �ϸ� a�� ��Ʈ�� 1�̸� �� ��ġ�� b�� ��Ʈ��, 0�̸� c�� ��Ʈ�� �����Ѵ�.
uint32_t Choose(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (~x & z);
}

//���ڷ� ���� �� ���� ���� ��ġ ��Ʈ�� 1�� ������ 1, 0�� ������ 0�� �ȴ�.
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

	nH[0] = BSigma_0(H[0]) + maj + s; // a' = (��0(a)  + Maj(a,b,c)) + (��1(e)  + Ch(e,f,g) + h + W0 + K0)
	nH[1] = H[0]; //  b' = a
	nH[2] = H[1]; //  c'=b
	nH[3] = H[2]; //  d'=c
	nH[4] = H[3] + s; // e' = (��1(e)  + Ch(e,f,g) + h + W0 + K0) + d 
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
	vector<uint8_t>message(begin(msg), end(msg)); // {"a","b","c"} ���� �ϳ��� 1byte ����(uint8_t = 1byte)
	cout << "\n";
	cout << "final hash : "<< SHA256(message) << "\n";

	return 0;
}

uint32_t ChangeEndian(uint32_t x)
{
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0xFF00FF);
	return (x << 16) | (x >> 16);
}

