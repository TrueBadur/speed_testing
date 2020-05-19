#include <iostream>
#include <cstring>
#include "structs.h"
#include <cmath>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <immintrin.h>

#define SIZE_OF_STAT 100000
#define WARM_UP_TIMES 10

void inline start_count(unsigned &cycles_high, unsigned &cycles_low)
{
	asm volatile (
	"CPUID\n\t"
	"RDTSC\n\t"
	"mov %%edx, %0\n\t"
	"mov %%eax, %1\n\t"
	: "=r" (cycles_high), "=r" (cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");

}

void inline stop_count(unsigned &cycles_high, unsigned &cycles_low)
{
	asm volatile (
	"RDTSCP\n\t"
	"mov %%edx, %0\n\t"
	"mov %%eax, %1\n\t"
	"CPUID\n\t"
	: "=r" (cycles_high), "=r" (cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
}

uint64_t inline convert_res(unsigned cycles_high, unsigned cycles_low)
{
	return ((uint64_t) cycles_high << 32u) | cycles_low;
}

struct StructResult{
	double mean;
	double std;
};

template<typename T, typename S, bool CheckCopy>
void inline
Filltimes(std::vector<uint64_t>& times, void (*f)(S *src, S* dst), T *src, T* dst)
{
	int i;
	uint64_t start, end;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	for (i = 0; i < WARM_UP_TIMES; i++)
	{
		start_count(cycles_high, cycles_low);
		stop_count(cycles_high1, cycles_low1);
	}
	for (i = 0; i < SIZE_OF_STAT; i++)
	{
		start_count(cycles_high, cycles_low);
		f((S*)src, (S*)dst);
		stop_count(cycles_high1, cycles_low1);
		start = convert_res(cycles_high, cycles_low);
		end = convert_res(cycles_high1, cycles_low1);
		times[i] = end - start;
		if (CheckCopy && std::memcmp((char*)src, (char*)dst, sizeof(T)) != 0){
			std::cerr << "Copy went wrong" << std::endl;
			break;
		}
	}
}

using NoPackDesc = Test1<std::int64_t, std::int64_t, std::int32_t, char, char>;
using NoPackMix = Test1<char, std::int64_t, std::int32_t, char, std::int64_t>;
using PackDesc = Test2<std::int64_t, std::int64_t, std::int32_t, char, char>;
using PackMix = Test2<char, std::int64_t, std::int32_t, char, std::int64_t>;
using ArrStruct = Test3<std::int64_t, std::int64_t, std::int32_t, char, char>;
using Results = std::unordered_map<std::string, std::unordered_map<std::string, StructResult>>;


template<typename S>
void StandartCopyTest(S* src, S* dst)
{
	for (int i = 0; i < 1; i++)
	{
		*dst = *src;
	}
}

template<typename S>
void MemcpyTest(S* src, S* dst)
{
	for (int i = 0; i < 1; i++)
	{
		std::memcpy(dst, src, sizeof(S));
	}
}

template<typename S>
void sseCopyTest(char* src, char* dst){
	for (int i = 0; i < 1; i++){
		char* target = src + sizeof(S);
		for (;src <= target - 16; src += 16, dst += 16){
			_mm_store_si128((__m128i*)dst, *(__m128i*)src);
		}
		for(;src <= target - 8; src += 8, dst += 8){
			_mm_storeu_si64(dst, *(__m128i*)src);
		}
		for(;src <= target - 4; src += 4, dst += 4){
			*(int*)dst = *(int*)src;
		}
		for(;src != target; src += 1, dst += 1){
			*dst = *src;
		}
	}
}

template<typename S>
inline void avxCopyTest(char* src, char* dst){
	for (int i = 0; i < 1; i++){
		char* target = src + sizeof(S);
		for (;src <= target - 32; src += 32, dst += 32){
			_mm256_store_si256((__m256i*)dst, *(__m256i*)src);
		}
		for (;src <= target - 16; src += 16, dst += 16){
			_mm_store_si128((__m128i*)dst, *(__m128i*)src);
		}
		for(;src <= target - 8; src += 8, dst += 8){
			_mm_storeu_si64(dst, *(__m128i*)src);
		}
		for(;src <= target - 4; src += 4, dst += 4){
			*(int*)dst = *(int*)src;
		}
		for(;src != target; src += 1, dst += 1){
			*dst = *src;
		}
	}
}

template<typename S>
inline void avx2CopyTest(char* src, char* dst){
	for (int i = 0; i < 1; i++){
		char* target = src + sizeof(S);
		for (;src <= target - 32; src += 32, dst += 32){
			_mm256_stream_si256((__m256i*)dst, *(__m256i*)src);
		}
		for (;src <= target - 16; src += 16, dst += 16){
			_mm_store_si128((__m128i*)dst, *(__m128i*)src);
		}
		for(;src <= target - 8; src += 8, dst += 8){
			_mm_storeu_si64(dst, *(__m128i*)src);
		}
		for(;src <= target - 4; src += 4, dst += 4){
			*(int*)dst = *(int*)src;
		}
		for(;src != target; src += 1, dst += 1){
			*dst = *src;
		}
	}
}

//template<typename S>
//inline void avx512CopyTest(char* src, char* dst){
//	for (int i = 0; i < 1; i++){
//		char* target = src + sizeof(S);
//
//		for (;src < src + sizeof(S); src += 16, dst += 16){
//			_mm512_stream_si512((__m512i*)dst, *(__m512i*)src);
//		}
//		for (;src <= target - 32; src += 32, dst += 32){
//			_mm256_store_si256((__m256i*)dst, *(__m256i*)src);
//		}
//		for (;src <= target - 16; src += 16, dst += 16){
//			_mm_store_si128((__m128i*)dst, *(__m128i*)src);
//		}
//		for(;src <= target - 8; src += 8, dst += 8){
//			_mm_storeu_si64(dst, *(__m128i*)src);
//		}
//		for(;src < target - 4; src += 4, dst += 4){
//			*(int*)dst = *(int*)src;
//		}
//		for(;src != target; src += 1, dst += 1){
//			*dst = *src;
//		}
//	}
//}

inline void Empty(int*, int*){
	for (int i = 0; i < 1; i++) {
	}
}

void proceed_data(std::vector<uint64_t>& inputs, StructResult& str_res)
{
	std::sort(inputs.begin(), inputs.end());
	auto size = (size_t)(SIZE_OF_STAT * .95f);
	size_t i;
	uint64_t min = inputs.front(), max = inputs.back();
	double variance = 0, std;
	for(i = 0; i < size; i++){
		str_res.mean += (inputs[i] - min);
	}
	str_res.mean = str_res.mean / size + min;
	for (i = 0; i < size; i++)
	{
		variance += std::pow(str_res.mean - (int64_t)inputs[i], 2);
	}
	variance /= size;
	str_res.std = sqrt(variance);
	std::cout << "\tMin value: " << min << "\n";
	std::cout << "\tMax value: " << max << "\n";
	std::cout << "\tMean value: " << str_res.mean << "\n";
	std::cout << "\tVariance: " << variance << "\n";
	std::cout << "\tSTD: " << std << std::endl;
}

template<typename T, bool CheckCopy = true, typename S>
StructResult TestFunction(void (*f)(S*, S*))
{
	StructResult res{0,0};
	auto volatile src = (T*)_mm_malloc(sizeof(T), 64);
	auto volatile dst = (T*)_mm_malloc(sizeof(T), 64);
	for(int i = 0; i < sizeof(T); i++){
		((char*)src)[i] = (char)i;
	}
	std::vector<uint64_t> times(SIZE_OF_STAT, 0);
	Filltimes<T, S, CheckCopy>(times, f, src, dst);
	_mm_free(src);
	_mm_free(dst);
	proceed_data(times, res);
	return res;
}

void print_final_results(Results& map){

	for (const auto& [struct_name, res_by_func] : map){
		double avg = 0, std = 0;
		std::cout << "Results for \033[0;32m" << struct_name << "\n";
		for (const auto& [func_name, res] : res_by_func){
			avg += res.mean;
			std += res.std;
			std::cout << "\t\033[0;34m" << func_name << "\033[0m mean result is \033[0;31m"
					  << res.mean << "\033[0;35m +/- " << res.std << "\033[0m\n";
		}
	}
	std::cout << "\033[0m";
}

template<typename T>
auto TestStruct (const std::string& struct_name = "[struct_name_not_provided]"){
	std::unordered_map<std::string, StructResult> res_by_func;
	std::cout << "\033[0;33mChecking " << struct_name << "with size " << sizeof(T) << std::endl;
	std::cout << "  Checking standard copy operator\n";
	res_by_func["StandardCopy"] = TestFunction<T>(StandartCopyTest<T>);
	std::cout << "  Checking memcpy\n";
	res_by_func["MemcpyTest"] = TestFunction<T>(MemcpyTest<T>);
	std::cout << "  Checking sse\n";
	res_by_func["sseCopyTest"] = TestFunction<T>(sseCopyTest<T>);
	std::cout << "  Checking avx\n";
	res_by_func["avxCopyTest"] = TestFunction<T>(avxCopyTest<T>);
	std::cout << "  Checking avx2\n";
	res_by_func["avx2CopyTest"] = TestFunction<T>(avx2CopyTest<T>);
//	std::cout << "  Checking avx512\n";
//	res_by_func["avx512CopyTest"] = TestFunction<T>(avx512CopyTest<T>);
	std::cout << "\033[0m";
	return res_by_func;
}

int main(int argc, char** argv)
{
	std::cout.setstate(std::ios_base::failbit);
	if (argc > 1 && strcmp(argv[1], "--more-info") == 0){
		std::cout.clear();
	}
	Results map;
	std::cout << "We are gona meassure this shit!\n";
	std::cout << "\033[0;33mReference value\033[0m\n";
	map["Reference"]["Empty"] = TestFunction<int, false, int>(Empty);
	map["NoPackDesc"] = TestStruct<NoPackDesc>("NoPackDesc");
	map["PackDesc"] = TestStruct<PackDesc>("PackDesc");
	map["NoPackMix"] = TestStruct<NoPackMix>("NoPackMix");
	map["PackMix"] = TestStruct<PackMix>("PackMix");
	map["ArrStruct"] = TestStruct<ArrStruct>("ArrStruct");
	std::cout.clear();
	print_final_results(map);
	return 0;
}
