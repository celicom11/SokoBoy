#pragma once
#define OUT
#define IN

#define USENEWDLM 1 //test new Static DL methods

#pragma warning(disable : 4619 4616 26812)

#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <list>
#include <queue>
#include <set>
#include <map>
#include <stack>
#include <ctime>
#include <stdlib.h>
#include <cassert>
#include <bitset>
#include <unordered_set>
#include <cmath>
using namespace std;

typedef const wchar_t* PCWSTR;
//
#include "SokoTypes.h"
//common macros
inline uint8_t _Bit1Pos(int64_t llBits) {//1 based! 0 if 0!
  uint8_t nPos = 0;
	while (llBits) {
		++nPos;
    if (llBits & 1)
      break;
		llBits >>= 1;
	}
	return nPos;
}
inline uint8_t _Popcnt64(int64_t llBits) {
	
#ifdef _WIN64
	return (uint8_t)__popcnt64(llBits);
#else
  // log(n) population count

  int64_t highBits = (llBits & 0xAAAAAAAAAAAAAAAA) >> 1;
  int64_t lowBits = llBits & 0x5555555555555555;
  int64_t bitSum = highBits + lowBits;

  highBits = (bitSum & 0xCCCCCCCCCCCCCCCC) >> 2;
  lowBits = bitSum & 0x3333333333333333;
  bitSum = highBits + lowBits;

  highBits = (bitSum & 0xF0F0F0F0F0F0F0F0) >> 4;
  lowBits = bitSum & 0x0F0F0F0F0F0F0F0F;
  bitSum = highBits + lowBits;

  highBits = (bitSum & 0xFF00FF00FF00FF00) >> 8;
  lowBits = bitSum & 0x00FF00FF00FF00FF;
  bitSum = highBits + lowBits;

  highBits = (bitSum & 0xFFFF0000FFFF0000) >> 16;
  lowBits = bitSum & 0x0000FFFF0000FFFF;
  bitSum = highBits + lowBits;

  highBits = (bitSum & 0xFFFFFFFF00000000) >> 32;
  lowBits = bitSum & 0x00000000FFFFFFFF;
  bitSum = highBits + lowBits;

  return (uint8_t)bitSum;

#endif
}
