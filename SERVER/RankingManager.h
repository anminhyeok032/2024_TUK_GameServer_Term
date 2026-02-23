#pragma once
#include "Session.h"

class RankingManager
{
private:
    std::vector<RankInfo> cached_ranking_; // 100명의 랭커 정보
    std::mutex cache_lock_;

public:
    static RankingManager* GetInstance() {
        static RankingManager instance;
        return &instance;
    }

    // 서버 시작 시 SQL 데이터를 Redis로 한 번 로딩 (초기화)
    void LoadAllRankingsFromSQL(SQLHDBC hdbc);

    // [주기적 호출] Redis -> 서버 메모리 캐시 갱신
    void UpdateRankingCache();

    // 클라이언트에게 캐싱된 랭킹 전송
    void SendRankingToPlayer(int session_id);

};