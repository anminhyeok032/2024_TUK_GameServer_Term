#include "Npc.h"

void Npc::DoRandomMove(int target_id)
{
	/*std::lock_guard<std::mutex> ml{ mut_lua_ };
	auto L = L_;
	lua_getglobal(L, "event_player_search");
	lua_pushnumber(L, target_id);
	lua_pcall(L, 1, 0, 0);*/
}

void Npc::SendAddObjectPacket(int c_id)
{
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
	//if (L_ == nullptr)
	//{
	//	SetAiLua();
	//}
	//OVER* over = new OVER;
	//switch (visual_)
	//{
	//case 1:	// Agro
	//{
	//	over->comp_key_ = KEY_NPC_RANDOM_MOVE;
	//	over->ai_target_c_id_ = id_;
	//	PostQueuedCompletionStatus(g_h_iocp, 1, id_, &over->over_);
	//	if (active_) return;
	//	bool prev_active = false;
	//	if (false == atomic_compare_exchange_strong(&active_, &prev_active, true))	return;
	//	AddTimer(id_, EV_NPC_RANDOM_MOVE, 1000, 0);
	//	break;
	//}
	//case 2: // peace
	//{
	//	break;
	//}
	//}
}

void Npc::SetAiLua()
{
	// ai로 돌아가는 것만 설정
	//auto L = L_ = luaL_newstate();
	//luaL_openlibs(L);
	//luaL_loadfile(L, "npc.lua");
	//lua_pcall(L, 0, 0, 0);

	//lua_getglobal(L, "set_uid");
	//lua_pushnumber(L, id_);
	//lua_pcall(L, 1, 0, 0);

	//lua_register(L, "API_Attack", API_Attack);
	//lua_register(L, "API_get_xy", API_get_xy);
}
