#include "Npc.h"

void Npc::DoRandomMove(int target_id)
{
	int final_target = target_id;

	// 현재 타겟이 유효한지 검증
	if (false == IsValidTarget(final_target))
	{
		// 타겟이 없거나 사라졌다면, 주변에서 새로운 타겟을 찾음 (어그로 변경)
		final_target = GetNearestPlayerId();
	}
	// 그래도 타겟이 없으면 다시 잠듦 (Sleep)
	if (final_target == -1)
	{
		active_ = false;
		return;
	}

	// Lua 스크립트 실행 (이동 방향 결정 등)
	{
		std::lock_guard<std::mutex> ml{ mut_lua_ };
		if (L_ == nullptr) SetAiLua();

		lua_getglobal(L_, "event_player_search");
		lua_pushnumber(L_, final_target); // 검증된 타겟 ID 전달

		if (lua_pcall(L_, 1, 0, 0) != LUA_OK) {
			std::cout << "Lua Error: " << lua_tostring(L_, -1) << std::endl;
			lua_pop(L_, 1);
		}
	}

	// 다음 행동 예약
	AddTimer(id_, EV_NPC_RANDOM_MOVE, 1000, final_target);
}

void Npc::Move(int dir)
{
	short dx = 0;
	short dy = 0;

	// 방향에 따른 좌표 변화량 (0:상, 1:하, 2:좌, 3:우)
	switch (dir) {
	case 0: dy = -1; break;
	case 1: dy = 1;  break;
	case 2: dx = -1; break;
	case 3: dx = 1;  break;
	}

	short new_x = x_ + dx;
	short new_y = y_ + dy;

	// 맵 범위 체크
	if (new_x < 0 || new_x >= W_WIDTH || new_y < 0 || new_y >= W_HEIGHT)
		return;

	// 좌표 갱신
	x_ = new_x;
	y_ = new_y;

	// 섹터 갱신 (섹터가 바뀌었으면 이동 처리)
	PutInSector();

	// 주변 플레이어에게 이동 패킷 전송 (Broadcasting)
	// 내 주변 섹터를 뒤져서 플레이어를 찾음
	for (auto& sector : around_sector_)
	{
		std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
		for (auto& p_id : g_ObjectSector[sector].sec_id_)
		{
			// 플레이어가 아니면 스킵
			if (false == IsPlayer(p_id)) continue;

			// 시야 내에 있는 플레이어에게만 전송
			if (CanSee(id_, p_id)) {
				// 해당 플레이어에게 NPC(나)가 이동했다는 패킷을 보냄
				objects[p_id]->SendMovePacket(id_);
			}
		}
	}
}

void Npc::SendAddObjectPacket(int c_id)
{
	// NPC는 뷰리스트를 관리하지 않지만(NPC끼리 통신 X),
	// 인터페이스 통일성을 위해 구현하거나 비워둠.
	// 만약 NPC가 어그로 대상을 기억해야 한다면 여기서 처리.
	mut_view_.lock();
	view_list_.insert(c_id);
	mut_view_.unlock();
}

void Npc::SendRemoveObjectPacket(int c_id)
{
	mut_view_.lock();
	view_list_.erase(c_id);
	mut_view_.unlock();
}

void Npc::SendAttackPacket(int attacker_id, int damaged_id, int exp)
{
	//SC_ATTACK_PACKET packet;
	//packet.size = sizeof(SC_ATTACK_PACKET);
	//packet.type = SC_ATTACK;
	//packet.attacker_id = attacker_id;
	//packet.damaged_id = damaged_id;
	//packet.max_hp = objects[damaged_id]->max_hp_;
	//packet.hp = objects[damaged_id]->hp_;
	//packet.exp = exp;
	//DoSend(&packet);
}

bool Npc::IsPlayerExist()
{
	for (auto& sector : around_sector_)
	{
		{
			// 섹터에 대한 lock
			std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
			for (auto& id : g_ObjectSector[sector].sec_id_)
			{
				if (false == IsPlayer(objects[id]->id_)) continue;
				if (true == CanSee(id_, objects[id]->id_))
					return true;
			}
		}
	}
	return false;
}

void Npc::WakeUpNpc(int p_id)
{
	if (L_ == nullptr)
	{
		SetAiLua();
	}
	// 이미 활동 중이면 리턴
	if (active_) return;

	bool prev_active = false;
	if (false == atomic_compare_exchange_strong(&active_, &prev_active, true)) return;

	OVER* over = g_sendPool.Acquire();
	switch (visual_)
	{
	case 1:	// Agro
	{
		over->comp_key_ = KEY_NPC_RANDOM_MOVE;
		over->ai_target_c_id_ = p_id;

		PostQueuedCompletionStatus(g_h_iocp, 1, id_, &over->over_);
		break;
	}
	case 2: // peace
	{
		break;
	}
	}
}

// 가장 가까운 플레이어 ID를 찾는 헬퍼 함수
int Npc::GetNearestPlayerId()
{
	int nearest_id = -1;
	int min_dist = INT_MAX;

	for (auto& sector : around_sector_)
	{
		std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
		for (auto& pid : g_ObjectSector[sector].sec_id_)
		{
			// 플레이어 검증 (ID 범위, nullptr 체크, InGame 상태 체크)
			if (pid < MAX_NPC || pid >= MAX_NPC + MAX_USER) continue;
			if (objects[pid] == nullptr) continue;
			if (objects[pid]->state_ != OS_INGAME) continue;

			// 시야 체크
			if (false == CanSee(id_, pid)) continue;

			// 거리 계산 (최단 거리 타겟 선정)
			int dist = (x_ - objects[pid]->x_) * (x_ - objects[pid]->x_)
				+ (y_ - objects[pid]->y_) * (y_ - objects[pid]->y_);

			if (dist < min_dist)
			{
				min_dist = dist;
				nearest_id = pid;
			}
		}
	}
	return nearest_id;
}

// 타겟이 여전히 유효한지(존재하고, 시야 내인지) 검사하는 함수
bool Npc::IsValidTarget(int target_id)
{
	if (target_id < MAX_NPC || target_id >= MAX_NPC + MAX_USER) return false;
	if (objects[target_id] == nullptr) return false;
	if (objects[target_id]->state_ != OS_INGAME) return false;

	// 시야 밖으로 나갔으면 타겟 해제 (또는 추적 거리 설정)
	if (false == CanSee(id_, target_id)) return false;

	return true;
}

void Npc::SetAiLua()
{
	// ai로 돌아가는 것만 설정
	auto L = L_ = luaL_newstate();
	luaL_openlibs(L);
	// 파일 로드 및 에러 체크
	if (luaL_loadfile(L_, "npc.lua") || lua_pcall(L_, 0, 0, 0)) {
		std::cout << "Lua Load Error: " << lua_tostring(L_, -1) << std::endl;
		L_ = nullptr;
		return;
	}

	lua_getglobal(L, "set_uid");
	lua_pushnumber(L, id_);
	if (lua_pcall(L_, 1, 0, 0) != LUA_OK) {
		std::cout << "Lua Error (set_uid): " << lua_tostring(L_, -1) << std::endl;
	}

	// C++ API 등록
	lua_register(L, "API_Attack", API_Attack);
	lua_register(L, "API_get_xy", API_get_xy);
	//이동 함수 등록
	lua_register(L, "API_Move", API_Move);
}

