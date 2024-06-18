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

	current_sector_ = new_sector;
	for (auto& sector : g_ObjectSector)
	{
		if (sector.second.sec_id_.find(id_) != sector.second.sec_id_.end()) {
			// 기존 섹터에서 플레이어 정보를 삭제
			sector.second.mut_sector_.lock();
			sector.second.sec_id_.erase(id_);
			sector.second.mut_sector_.unlock();
			break;
		}
	}

	// 현재 sector에 _id 저장
	g_ObjectSector[new_sector].mut_sector_.lock();
	g_ObjectSector[new_sector].sec_id_.insert(id_);
	g_ObjectSector[new_sector].mut_sector_.unlock();
}
