#include "global.h"
#include "Session.h"

void AddTimer(int id, EVENT_TYPE type, int ms, int target_id)
{
	EVENT ev{ id, std::chrono::system_clock::now() + std::chrono::milliseconds(ms), type, target_id };
	g_event_queue.push(ev);
}

void DoAITimer() {
	// 이벤트 체크용 변수들
	EVENT next_event;
	bool has_next_event = false;

	while (true)
	{
		EVENT ev;
		auto current_time = std::chrono::system_clock::now();

		// 기존 시간 안된 뽑은 이벤트 있으면 ev에 넣어 먼저 처리
		if (has_next_event) {
			ev = next_event;
			has_next_event = false;
		}
		else if (!g_event_queue.try_pop(ev)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// try_pop으로 뽑은 이벤트가 현재 시간보다 뒤라면 next_event에 넣고 슬립
		if (ev.wakeup_time_ > current_time) {
			next_event = ev;
			has_next_event = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		// 이벤트 처리 로직
		switch (ev.e_type_) {
		case EV_ATTACK:
		{
			OVER* ov = new OVER;
			break;
		}
		case EV_NPC_RANDOM_MOVE:
		{
			OVER* ov = new OVER;
			ov->comp_key_ = KEY_NPC_RANDOM_MOVE;
			ov->ai_target_c_id_ = ev.target_id_;
			PostQueuedCompletionStatus(g_h_iocp, 1, ev.id_, &ov->over_);
			break;
		}

		}


	}
}

int API_get_xy(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = objects[user_id]->x_;
	int y = objects[user_id]->y_;
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 1;
}

int API_Attack(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -2);
	int target_id = (int)lua_tointeger(L, -1);

	lua_pop(L, 3);

	objects[target_id]->hp_ -= 10;
	objects[target_id]->SendStatChangePacket();


	// 공격 판정 맞는 사람 입장 view_list 브로드 캐스팅
	for (auto& view_list : objects[target_id]->view_list_)
	{
		objects[view_list]->SendAttackPacket(my_id, target_id, 0);
	}

	//if (objects[id]->hp_ <= 0)
	//{
	//	// 죽음 처리
	//	objects[id]->hp_ = 0;
	//	objects[id]->state_ = OS_DEAD;
	//	int getting_exp = objects[id]->level_ * objects[id]->level_ * 2;
	//	exp_ += getting_exp;
	//	int required_exp = 100 * pow(2, level_ - 1);
	//	if (exp_ >= required_exp)
	//	{
	//		level_++;
	//		exp_ -= required_exp;
	//		std::cout << "Level up - " << level_ << "!" << std::endl;
	//	}
	//	SendStatChangePacket();
	//	for (auto& view_list : objects[id]->view_list_)
	//	{
	//		objects[view_list]->SendAttackPacket(id_, objects[id]->id_, getting_exp);
	//	}

	//	// Npc의 뷰리스트에 있는 Player한테만 보냄
	//	for (auto& view_list : objects[id]->view_list_)
	//	{
	//		objects[view_list]->SendRemoveObjectPacket(id);
	//	}
	//}
	return 0;
}