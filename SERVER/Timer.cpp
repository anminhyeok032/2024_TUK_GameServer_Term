#include "global.h"
#include "Session.h"
#include "Npc.h"

void AddTimer(int id, EVENT_TYPE type, int ms, int target_id)
{
	EVENT ev{ id, std::chrono::system_clock::now() + std::chrono::milliseconds(ms), type, target_id };
	g_event_queue.push(ev);
}

void DoAITimer() {

	while (true)
	{
		EVENT ev;
		auto current_time = std::chrono::system_clock::now();


		if (!g_event_queue.try_pop(ev)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// try_pop으로 뽑은 이벤트가 현재 시간보다 뒤라면 next_event에 넣고 슬립
		if (ev.wakeup_time_ > current_time) {
			g_event_queue.push(ev);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// 이벤트 처리 로직
		switch (ev.e_type_) {
		case EV_ATTACK:
		{
			//OVER* ov = new OVER;
			break;
		}
		case EV_MOVE_TO_PLAYER:
		{
			//OVER* ov = new OVER;
			OVER* ov = g_sendPool.Acquire();
			ov->comp_key_ = KEY_NPC_MOVE_TO_PLAYER;
			ov->ai_target_c_id_ = ev.target_id_;
			PostQueuedCompletionStatus(g_h_iocp, 1, ev.id_, &ov->over_);
			break;
		}

		case EV_NPC_RANDOM_MOVE:
		{
			//OVER* ov = new OVER;
			OVER* ov = g_sendPool.Acquire();
			ov->comp_key_ = KEY_NPC_RANDOM_MOVE;
			ov->ai_target_c_id_ = ev.target_id_;
			PostQueuedCompletionStatus(g_h_iocp, 1, ev.id_, &ov->over_);
			break;
		}

		}


	}
}

// Lua 호출: API_get_xy(id)
int API_get_xy(lua_State* L)
{
	// 인자 가져오기 (스택 1번: user_id)
	int user_id = (int)lua_tointeger(L, 1);

	// 유효성 검사
	if (user_id < 0 || user_id >= MAX_NPC + MAX_USER || objects[user_id] == nullptr) {
		lua_pushinteger(L, -1);
		lua_pushinteger(L, -1);
		return 2;
	}

	int x = objects[user_id]->x_;
	int y = objects[user_id]->y_;

	// 결과 푸시
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);

	return 2; // 리턴값 개수는 2개 (x, y)
}

// Lua 호출: API_Move(my_id, direction)
int API_Move(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, 1); // 첫 번째 인자
	int dir = (int)lua_tointeger(L, 2);   // 두 번째 인자

	// Npc 객체인지 확인하고 이동 함수 호출
	if (my_id >= 0 && my_id < MAX_NPC && objects[my_id]) {
		Npc* npc = dynamic_cast<Npc*>(objects[my_id].get());
		if (npc) {
			npc->Move(dir);
		}
	}
	return 0; // 반환값 없음
}

int API_Attack(lua_State* L)
{
	//int my_id = (int)lua_tointeger(L, -2);
	//int target_id = (int)lua_tointeger(L, -1);

	//lua_pop(L, 3);

	//objects[target_id]->hp_ -= 10;
	//objects[target_id]->SendStatChangePacket();


	//// 공격 판정 맞는 사람 입장 view_list 브로드 캐스팅
	//for (auto& view_list : objects[target_id]->view_list_)
	//{
	//	objects[view_list]->SendAttackPacket(my_id, target_id, 0);
	//}

	////if (objects[id]->hp_ <= 0)
	////{
	////	// 죽음 처리
	////	objects[id]->hp_ = 0;
	////	objects[id]->state_ = OS_DEAD;
	////	int getting_exp = objects[id]->level_ * objects[id]->level_ * 2;
	////	exp_ += getting_exp;
	////	int required_exp = 100 * pow(2, level_ - 1);
	////	if (exp_ >= required_exp)
	////	{
	////		level_++;
	////		exp_ -= required_exp;
	////		std::cout << "Level up - " << level_ << "!" << std::endl;
	////	}
	////	SendStatChangePacket();
	////	for (auto& view_list : objects[id]->view_list_)
	////	{
	////		objects[view_list]->SendAttackPacket(id_, objects[id]->id_, getting_exp);
	////	}

	////	// Npc의 뷰리스트에 있는 Player한테만 보냄
	////	for (auto& view_list : objects[id]->view_list_)
	////	{
	////		objects[view_list]->SendRemoveObjectPacket(id);
	////	}
	////}


	return 0;
}