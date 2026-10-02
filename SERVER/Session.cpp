#include "Session.h"

void SESSION::PutInSector()
{
	std::pair<int, int> new_sector = { x_ / SEC_ROW, y_ / SEC_COL };

	// around_sector_에 주변 섹터 정보 저장
	around_sector_.clear();
	around_sector_.insert(new_sector);
	std::vector<std::pair<int, int>> offsets = 
	{
		{ -SEC_RANGE, -SEC_RANGE }, { -SEC_RANGE, SEC_RANGE },
		{ SEC_RANGE, -SEC_RANGE }, { SEC_RANGE, SEC_RANGE }
	};
	for (const auto& offset : offsets) 
	{
		around_sector_.insert({ (x_ + offset.first / 2) / SEC_ROW, (y_ + offset.second / 2) / SEC_COL });
	}


	// 기존 섹터와 동일한지 검사
	if (current_sector_ == new_sector) 
	{
		return; // 같다면 아무것도 하지 않고 반환
	}

	// 객체는 항상 current_sector_ 섹터 하나에만 들어 있으므로 다른 섹터를 뒤질 필요가 없다.
	// 덮어쓰기 전에 이전 섹터를 기억해 둔다.
	std::pair<int, int> old_sector = current_sector_;
	current_sector_ = new_sector;

	// 새 섹터에 먼저 넣고 이전 섹터에서 빼기
	{
		std::lock_guard<std::mutex> sec_l(g_ObjectSector[new_sector].mut_sector_);
		g_ObjectSector[new_sector].sec_id_.insert(id_);
	}
	if (old_sector.first >= 0)	// 아직 어느 섹터에도 들어간 적이 없으면 {-99, -99}
	{
		std::lock_guard<std::mutex> sec_l(g_ObjectSector[old_sector].mut_sector_);
		g_ObjectSector[old_sector].sec_id_.erase(id_);
	}
}
