-- 이 스크립트의 상태는 워커 스레드마다 하나씩 만들어져 여러 NPC가 같이 사용
-- 전역 변수에 NPC별 값을 저장하지 말 것 (다음에 실행되는 다른 NPC가 그 값을 보게 된다)

-- 방향 상수 정의 (C++ enum과 맞춰야 함)
local DIR_UP = 0
local DIR_DOWN = 1
local DIR_LEFT = 2
local DIR_RIGHT = 3

function event_player_search(my_id, player)
    -- 좌표 가져오기
    local p_x, p_y = API_get_xy(player)
    local my_x, my_y = API_get_xy(my_id)

    -- 거리 차이 계산
    local diff_x = p_x - my_x
    local diff_y = p_y - my_y

    -- 바로 옆(공격 사거리)인지 확인 (상하좌우 1칸)
    if (math.abs(diff_x) + math.abs(diff_y)) <= 1 then
        API_Attack(my_id, player)
        return -- 공격했으면 이동하지 않고 종료
    end

    -- 추적 로직 (이동)
    -- X축 거리가 Y축 거리보다 멀면 X축으로 먼저 이동, 아니면 Y축 이동
    if math.abs(diff_x) > math.abs(diff_y) then
        if diff_x > 0 then
            API_Move(my_id, DIR_RIGHT)
        else
            API_Move(my_id, DIR_LEFT)
        end
    else
        if diff_y > 0 then
            API_Move(my_id, DIR_DOWN)
        else
            API_Move(my_id, DIR_UP)
        end
    end
end