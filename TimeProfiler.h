#pragma once
#include <intrin.h>
#include <chrono>
using namespace std::chrono;

//Collector Interface
struct __declspec(novtable) ITPCollector abstract {
  virtual ~ITPCollector(){}
  virtual void AddTime(PCSTR szFunc, uint64_t ullTimeDiff) = 0;
};

class CTimeProfiler {
  PCSTR         m_szFuncName{ NULL }; //external/dnd!
  ITPCollector* m_pTPC{ nullptr };
  time_point<steady_clock> m_tpStart;
public:
//CTOR/DTOR
  CTimeProfiler(const char* szFuncName, ITPCollector* pTPC) : 
    m_szFuncName(szFuncName), m_pTPC(pTPC) {
    m_tpStart = high_resolution_clock::now();
  }
  ~CTimeProfiler() {
    time_point<steady_clock> tpEnd = high_resolution_clock::now();
    uint64_t ullDiff = duration_cast<nanoseconds>(tpEnd - m_tpStart).count();
    m_pTPC->AddTime(m_szFuncName, ullDiff);
  }
};
class CTPCollector : public ITPCollector {
  map<string, uint64_t>  m_mapSLL;
public:
  CTPCollector() {
  }
  void AddTime(PCSTR szFunc, uint64_t ullTimeDiff) override {
    m_mapSLL[szFunc] += ullTimeDiff;
  }
  string ToString() const {
    string sRet;
    for (const auto& itSLL : m_mapSLL) {
      sRet += itSLL.first + ": " + std::to_string(uint64_t(itSLL.second/1000000000ull)) + "\n";
    }
    sRet.pop_back();
    return sRet;
  }
};