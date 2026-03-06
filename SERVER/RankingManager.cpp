#include "RankingManager.h"


void DisplayDBError(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

bool IsValidName(const std::string& name)
{
    if (name.empty()) return false;

    // 이름이 너무 길면 무시 (혹은 잘라서 쓸거면 true)
    if (name.length() > NAME_SIZE) return false;

    // 제어 문자(0x00~0x1F, 줄바꿈 등)가 포함되어 있으면 깨진 것으로 간주
    for (unsigned char c : name) {
        // 한글(음수값 char)은 건너뛰고, ASCII 제어 문자만 체크
        if (c > 0 && c < 32) return false;
    }

    return true;
}

// 오른쪽 공백 제거
inline std::string RTrim(std::string s) 
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
        {
            return !std::isspace(ch);
        }).base(), s.end());
    return s;
}

void RankingManager::LoadAllRankingsFromSQL(SQLHDBC hdbc)
{
    if (!g_redis_client || !g_redis_client->is_connected()) return;
    std::cout << "[Ranking] SQL -> Redis 동기화 시작...\n";

    SQLHSTMT hstmt;
    if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS) {
        std::cerr << "SQL Alloc Handle Failed\n";
        return;
    }

    SQLWCHAR query[] = L"SELECT user_id, user_exp, user_level FROM [dbo].[user_term_table] ORDER BY user_exp DESC";

    RETCODE ret = SQLExecDirect(hstmt, query, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) 
    {
        std::cerr << "SQL Execute Failed! Error Code: " << ret << "\n";
        DisplayDBError(hstmt, SQL_HANDLE_STMT, ret);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return;
    }

    SQLWCHAR name[NAME_SIZE];
    SQLINTEGER exp;
    SQLINTEGER level;
    SQLLEN cbName, cbExp, cbLevel;

    SQLBindCol(hstmt, 1, SQL_C_WCHAR, name, sizeof(name), &cbName);
    SQLBindCol(hstmt, 2, SQL_C_SLONG, &exp, 0, &cbExp);
    SQLBindCol(hstmt, 3, SQL_C_SLONG, &level, 0, &cbLevel);

    int count = 0;

    // 루프 진입
    while (true)
    {
        ret = SQLFetch(hstmt);

        // 데이터가 더 이상 없으면 종료
        if (ret == SQL_NO_DATA) break;

        // 에러가 났으면 로그 찍고 종료
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cerr << "SQLFetch Failed at row " << count << "\n";
            DisplayDBError(hstmt, SQL_HANDLE_STMT, ret);
            break;
        }

        // 정상 데이터 처리
        std::string name_str = WStringToString(name);
        name_str = RTrim(name_str);
        std::string key = "User:" + name_str;

        // 랭킹(ZSET) 등록 - multimap 사용
        std::multimap<std::string, std::string> zadd_data;
        zadd_data.insert({ std::to_string(exp), name_str });
        g_redis_client->zadd("Leaderboard", {}, zadd_data);

        // 상세정보(Hash) 등록
        g_redis_client->hset(key, "level", std::to_string(level));

        // TTL 설정 (1주일)
        g_redis_client->expire(key, 604800);

        count++;
        if (count % 1000 == 0) g_redis_client->sync_commit();
    }

    g_redis_client->sync_commit();
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    std::cout << "[Ranking] 동기화 완료 (" << count << "명)\n";
}

// 2. Redis에서 Top 100 가져오기 (10초마다 호출)
void RankingManager::UpdateRankingCache()
{
    std::vector<std::string> valid_names;
    const int TARGET_COUNT = 100; // 목표 인원
    int start_index = 0;          // Redis 조회 시작 인덱스

    // 목표 인원을 채울 때까지 반복 (또는 DB 끝까지)
    while (valid_names.size() < TARGET_COUNT)
    {
        // 부족한 인원 수 계산
        int needed = TARGET_COUNT - (int)valid_names.size();

        // 딱 부족한 만큼만 요청하면 또 깨진 이름이 나올 경우 다시 요청해야 함
        int fetch_count = needed + 10;
        int end_index = start_index + fetch_count - 1;

        // 이름 목록 가져오기 (Range 요청)
        auto zrev_future = g_redis_client->zrevrange("Leaderboard", start_index, end_index);
        g_redis_client->sync_commit(); // 대기

        auto reply = zrev_future.get();

        // 에러거나 배열이 아니면 중단
        if (reply.is_null() || !reply.is_array()) break;

        auto arr = reply.as_array();

        // 데이터가 아예 없으면 (끝까지 다 읽음) 루프 종료
        if (arr.empty()) break;

        // 유효성 검사 및 저장
        for (const auto& item : arr)
        {
            // 이미 목표를 다 채웠으면 그만 담기
            if (valid_names.size() >= TARGET_COUNT) break;

            std::string raw_name = item.as_string();

            // 이름이 유효하지 않으면 건너뜀 (담지 않음)
            if (IsValidName(raw_name) == false)
            {
                // Redis에서 삭제: g_redis_client->zrem("Leaderboard", { raw_name });
                continue;
            }

            valid_names.push_back(raw_name);
        }

        // 다음 루프를 위해 시작 인덱스 갱신
        // 이번에 읽어온 개수만큼 인덱스를 뒤로 밈
        start_index += (int)arr.size();

        // 요청한 개수보다 적게 왔다는 건, DB에 남은 사람이 더 없다는 뜻
        if (arr.size() < fetch_count) break;
    }

    if (valid_names.empty()) return;

    // 각 유저의 레벨 정보 가져오기 (파이프라이닝)
    std::vector<std::future<cpp_redis::reply>> futures;

    for (const auto& name : valid_names)
    {
        futures.push_back(g_redis_client->hget("User:" + name, "level"));
    }
    // blocking으로 값 기다림
    g_redis_client->sync_commit();


    // 결과 조립 및 캐시 갱신
    std::vector<RankInfo> temp_ranks;
    int rank_counter = 1;

    for (size_t i = 0; i < valid_names.size(); ++i)
    {
        RankInfo info;
        strncpy_s(info.name, sizeof(info.name), valid_names[i].c_str(), _TRUNCATE);
        info.rank = rank_counter++;

        auto result = futures[i].get();

        if (result.is_string()) 
        {
            info.level = std::stoi(result.as_string());
        }
        else
        {
            info.level = 0; // 데이터가 없거나 에러 시 기본값
        }

        // std::cout << "Rank " << info.rank << " : " << info.name << " (Lv." << info.level << ")\n";
        temp_ranks.push_back(info);
    }

    // C++에서 재정렬 (Level 내림차순 -> Name 오름차순)
    std::sort(temp_ranks.begin(), temp_ranks.end(), [](const RankInfo& a, const RankInfo& b) {
        // 레벨 비교 (높은 게 위로)
        if (a.level != b.level)
            return a.level > b.level;

        // 레벨이 같으면 이름 비교 (사전순, strcmp)
        return strcmp(a.name, b.name) < 0;
        });

    // 정렬된 순서대로 랭킹 번호 부여 (1~100)
    for (int i = 0; i < temp_ranks.size(); ++i)
    {
        temp_ranks[i].rank = i + 1;
    }

    // 캐시 교체
    {
        std::lock_guard<std::mutex> lock(cache_lock_);
        cached_ranking_ = temp_ranks;
    }
}

// 랭킹 패킷 전송
void RankingManager::SendRankingToPlayer(int session_id)
{
    SC_RANKING_PACKET packet;
    packet.size = sizeof(SC_RANKING_PACKET);
    packet.type = SC_RANKING; // 정의된 상수 사용

    {
        std::lock_guard<std::mutex> lock(cache_lock_);
        packet.count = (int)cached_ranking_.size();
        if (packet.count > 100) packet.count = 100;

        // 메모리 복사
        if (packet.count > 0)
        {
            memcpy(packet.ranks, cached_ranking_.data(), sizeof(RankInfo) * packet.count);
        }
    }

    // 세션이 유효하다면 전송
    if (objects[session_id])
    {
        objects[session_id]->DoSend(&packet);
    }
}