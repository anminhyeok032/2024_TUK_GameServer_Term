#include "Session.h"

void SESSION::PutInSector()
{
	std::pair<int, int> new_sector = { x_ / SEC_ROW, y_ / SEC_COL };

	// around_sector_�� �ֺ� ���� ���� ����
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


	// ���� ���Ϳ� �������� �˻�
	if (current_sector_ == new_sector) 
	{
		return; // ���ٸ� �ƹ��͵� ���� �ʰ� ��ȯ
	}

	current_sector_ = new_sector;
	for (auto& sector : g_ObjectSector)
	{
		if (sector.second.sec_id_.find(id_) != sector.second.sec_id_.end()) {
			// ���� ���Ϳ��� �÷��̾� ������ ����
			sector.second.mut_sector_.lock();
			sector.second.sec_id_.erase(id_);
			sector.second.mut_sector_.unlock();
			break;
		}
	}

	// ���� sector�� _id ����
	g_ObjectSector[new_sector].mut_sector_.lock();
	g_ObjectSector[new_sector].sec_id_.insert(id_);
	g_ObjectSector[new_sector].mut_sector_.unlock();
}
