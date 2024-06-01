#pragma once
//simple container wrappers
class SQVec : public IStageQueue {
  vector<Stage>   m_vStages;
public:
  SQVec() = default;
  ~SQVec() = default;
//IStageQueue
	bool IsEmpty() const override { return m_vStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_vStages.size(); }
	bool HasStage(const Stage& stage) const override {
		return std::find(m_vStages.begin(), m_vStages.end(), stage) != m_vStages.end();
	}
	void Clear() override { m_vStages.clear(); }
	const Stage& Top() const override {
		return m_vStages.back();
	}
	Stage Pop() override {
		Stage ret = m_vStages.back();
		m_vStages.pop_back();
		return ret;
	};
	void Push(const Stage& stage ) override {
		m_vStages.push_back(stage);
	}
	const Stage* Parent(const Stage& stage) const override {
		if (stage.nPIdx <= 0 || stage.nPIdx - 1 >= m_vStages.size()) 
			return nullptr;
		return &m_vStages[stage.nPIdx - 1];
	}
};
class SQDeque : public IStageQueue {
	deque<Stage>   m_dqStages;
public:
	SQDeque() = default;
	~SQDeque() = default;
	//IStageQueue
	bool IsEmpty() const override { return m_dqStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_dqStages.size(); }
	bool HasStage(const Stage& stage) const override {
		return std::find(m_dqStages.begin(), m_dqStages.end(), stage) != m_dqStages.end();
	}
	void Clear() override { m_dqStages.clear(); }
	const Stage& Top() const override {
		return m_dqStages.front();
	}
	Stage Pop() override {
		Stage ret = m_dqStages.front();
		m_dqStages.pop_front();
		return ret;
	};
	void Push(const Stage& stage) override {
		m_dqStages.push_back(stage);
	}
	const Stage* Parent(const Stage& stage) const override {
		if (stage.nPIdx <= 0 || stage.nPIdx - 1 >= m_dqStages.size())
			return nullptr;
		return &m_dqStages[stage.nPIdx - 1];
	}
};
class SQList : public IStageQueue {
	list<Stage>   m_lstStages;
public:
	SQList() = default;
	~SQList() = default;
	//IStageQueue
	bool IsEmpty() const override { return m_lstStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_lstStages.size(); }
	bool HasStage(const Stage& stage) const override {
		return std::find(m_lstStages.begin(), m_lstStages.end(), stage) != m_lstStages.end();
	}
	void Clear() override { m_lstStages.clear(); }
	const Stage& Top() const override {
		return m_lstStages.back();
	}
	Stage Pop() override {
		Stage ret = m_lstStages.back();
		m_lstStages.pop_back();
		return ret;
	};
	void Push(const Stage& stage) override {
		m_lstStages.push_back(stage);
	}
	const Stage* Parent(const Stage& stage) const override {
		if (stage.nPIdx <= 0 || stage.nPIdx - 1 >= m_lstStages.size())
			return nullptr;
		auto it = m_lstStages.begin();
		int nDist = stage.nPIdx;
		while (--nDist) {
			++it;
		}
		return &(*it);
	}
};
//extended container with stage's weights/ordering
class Sokoban;
//Stack w sorted by weight stages
class SQStackW : public IStageQueue {
	const Sokoban&	m_Sokoban;		//field/precalcs
	vector<Stage>		m_vStages;
	vector<Stage>		m_vNewStages;	//intermediate SORTED vec for good after-push stages
public:
	SQStackW() = delete;
	SQStackW(const Sokoban& sokoban) :m_Sokoban(sokoban) {};
	~SQStackW() = default;
	//IStageQueue
	bool IsEmpty() const override { return m_vNewStages.empty() && m_vStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_vStages.size(); }
	bool HasStage(const Stage& stage) const override {
		return std::find(m_vStages.begin(), m_vStages.end(), stage) != m_vStages.end() ||
			std::find(m_vNewStages.begin(), m_vNewStages.end(), stage) != m_vNewStages.end();
	}
	void Clear() override {
		m_vStages.clear(); m_vNewStages.clear();
	}
	const Stage& Top() const override {
		return m_vStages.back();
	}
	Stage Pop() override {
		//push accumulated stages first
		if (m_vNewStages.size() * 10 > m_vStages.size()) {//TEST
			for (const Stage& stgNew : m_vNewStages) {
				m_vStages.push_back(stgNew);
			}
			m_vNewStages.clear();
		}
		Stage ret = m_vStages.back();
		m_vStages.pop_back();
		return ret;
	};
	void Push(const Stage& stage) override {
		Stage stgNew = stage;
		m_Sokoban.UpdateStageWeight(stgNew);
		if (m_Sokoban.Cfg().nDFS_MaxDepth && m_Sokoban.Cfg().nDFS_MaxDepth < m_Sokoban.Depth(stage))
			return; //cutoff!
		if (stgNew.nWeight > m_Sokoban.ParentWeight(stage)+4)
			return;//cutoff 2!
		uint32_t nDec = (uint32_t)m_vNewStages.size();
		for (;nDec; --nDec) {
			const Stage& stg = m_vNewStages[nDec - 1];
			if (stg.nWeight >= stgNew.nWeight)
				break;
		}
		m_vNewStages.insert(m_vNewStages.begin()+ nDec, stgNew);
	}
	const Stage* Parent(const Stage& stage) const override {
		if (stage.nPIdx <= 0 || stage.nPIdx - 1 >= m_vStages.size())
			return nullptr;
		return &m_vStages[stage.nPIdx - 1];
	}
};
//ordered set of stages sorted by weights
class SQSetW : public IStageQueue {
	static bool _StageCmp(const Stage& stg1, const Stage& stg2) noexcept {
		return stg1.nWeight < stg2.nWeight;
	};
	const Sokoban& m_Sokoban;		//field/precalcs
	set< Stage, decltype(&_StageCmp)> m_setStages;
public:
	SQSetW() = delete;
	SQSetW(const Sokoban& sokoban) : m_Sokoban(sokoban), m_setStages(&_StageCmp) {};
	~SQSetW() = default;
	//IStageQueue
	bool IsEmpty() const override { return m_setStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_setStages.size(); }
	bool HasStage(const Stage& stage) const override {
		return m_setStages.find(stage) != m_setStages.end();
	}
	void Clear() override { m_setStages.clear(); }
	const Stage& Top() const override {
		return *m_setStages.begin();
	}
	Stage Pop() override {
		Stage ret = *m_setStages.begin();
		m_setStages.erase(m_setStages.begin());
		return ret;
	};
	void Push(const Stage& stage) override {
		Stage stgNew = stage;
		m_Sokoban.UpdateStageWeight(stgNew);
		m_setStages.insert(stgNew);
	}
	const Stage* Parent(const Stage& stage) const override {
		assert(0);//NI
		return nullptr;
	}
};
class SQPQueue : 	public IStageQueue {
	template <class T, class S, class C>
	const S& Container(const priority_queue<T, S, C>& q) const {
		struct HackedQueue : private priority_queue<T, S, C> {
			static const S& Container(const priority_queue<T, S, C>& q) {
				return q.* & HackedQueue::c;
			}
		};
		return HackedQueue::Container(q);
	}
	static bool _StageCmp(const Stage& stg1, const Stage& stg2) noexcept {
		return stg1.nWeight > stg2.nWeight || (stg1.nWeight == stg2.nWeight && stg1.nPIdx> stg2.nPIdx);
	};
	const Sokoban& m_Sokoban;		//field/precalcs
	priority_queue<Stage, vector<Stage>, decltype(&_StageCmp)> m_pqStages;
public:
	SQPQueue() = delete;
	SQPQueue(const Sokoban& sokoban) :m_Sokoban(sokoban), m_pqStages(_StageCmp) {};
	~SQPQueue() = default;
	//IStageQueue
	bool IsEmpty() const override { return m_pqStages.empty(); }
	uint32_t Size() const override { return (uint32_t)m_pqStages.size(); }
	bool HasStage(const Stage& stage) const override {
		const vector<Stage>& vMyC = Container(m_pqStages);
		return std::find(vMyC.begin(), vMyC.end(), stage) != vMyC.end();
	}
	void Clear() override { m_pqStages = priority_queue<Stage, vector<Stage>, decltype(&_StageCmp)>(_StageCmp); }
	const Stage& Top() const override {
		return m_pqStages.top();
	}
	Stage Pop() override {
		Stage ret = m_pqStages.top();
		m_pqStages.pop();
		return ret;
	};
	void Push(const Stage& stage) override {
		Stage stgNew = stage;
		m_Sokoban.UpdateStageWeight(stgNew);
		m_pqStages.push(stgNew);
	}
	const Stage* Parent(const Stage& stage) const override {
		assert(0);//NI
		return nullptr;
	}
};

