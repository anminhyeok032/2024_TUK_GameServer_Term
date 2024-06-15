#include "Npc.h"

void Npc::DoRandomMove()
{
	std::cout << "Npc::DoRandomMove()" << std::endl;
}

bool Npc::IsPlayerExist()
{
	for (auto& sector : around_sector_)
	{
		{
			// ���Ϳ� ���� lock
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
