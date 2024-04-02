#pragma once
#include "Sokoban.h"

class CBM45_Up : public IBitMgr45 {
//DATA
  const Sokoban&  m_Sokoban;
  const Stage&    m_Stage;
public:
//CTOR/DTOR
  CBM45_Up(const Sokoban& sb, const Stage& stage) : m_Sokoban(sb), m_Stage(stage) {};
  ~CBM45_Up() = default;
//IBitMgr45 impl
  int64_t GetBits23() override;
};
class CBM45_Dn : public IBitMgr45 {
  //DATA
  const Sokoban& m_Sokoban;
  const Stage& m_Stage;
public:
  //CTOR/DTOR
  CBM45_Dn(const Sokoban& sb, const Stage& stage) : m_Sokoban(sb), m_Stage(stage) {};
  ~CBM45_Dn() = default;
  //IBitMgr45 impl
  int64_t GetBits23() override;
};
class CBM45_Lt : public IBitMgr45 {
  //DATA
  const Sokoban& m_Sokoban;
  const Stage& m_Stage;
public:
  //CTOR/DTOR
  CBM45_Lt(const Sokoban& sb, const Stage& stage) : m_Sokoban(sb), m_Stage(stage) {};
  ~CBM45_Lt() = default;
  //IBitMgr45 impl
  int64_t GetBits23() override;
};
class CBM45_Rt : public IBitMgr45 {
  //DATA
  const Sokoban& m_Sokoban;
  const Stage& m_Stage;
public:
  //CTOR/DTOR
  CBM45_Rt(const Sokoban& sb, const Stage& stage) : m_Sokoban(sb), m_Stage(stage) {};
  ~CBM45_Rt() = default;
  //IBitMgr45 impl
  int64_t GetBits23() override;
};