#include "Npc.h"

void Npc::DoRandomMove()
{
	std::cout << "Npc::DoRandomMove()" << std::endl;
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
