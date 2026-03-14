#include "Inventory.h"


Inventory::Inventory(int owner_id) : owner_id_(owner_id)
{
	// 그리드 초기화
	for (int y = 0; y < INV_MAX_ROW; ++y) 
	{
		for (int x = 0; x < INV_MAX_COL; ++x) 
		{
			grid_[y][x] = 0;
		}
	}
}

Inventory::~Inventory()
{
	// 할당된 아이템 메모리 해제
	for (auto& pair : items_)
	{
		delete pair.second;
	}
	items_.clear();
}

ItemTemplate Inventory::GetItemTemplate(int template_id)
{
	ItemTemplate info;
	info.template_id = template_id;
	info.width = 1; 
	info.height = 1;

	switch (template_id)
	{
		// ---- 2x3 ----
	case 1001:                          // 대검
		info.width = 2; info.height = 3; break;

		// ---- 2x2 ----
	case 1002:                          // 방패
		info.width = 2; info.height = 2; break;

		// ---- 1x2 ----
	case 1003:                          // 장검
	case 1004:                          // 단검
	case 1005:                          // 창
	case 1006:                          // 단창
		info.width = 1; info.height = 2; break;

		// ---- 2x1 ----
	case 1013:                          // 완드
	case 1014:                          // 막대
		info.width = 2; info.height = 1; break;

		// ---- 1x1 (기본값과 동일, 명시적 나열) ----
	case 1007:                          // 표창
	case 1008:                          // 단도
	case 1009:                          // 곡검
	case 1010:                          // 송곳
	case 1011:                          // 바늘
	case 1012:                          // 건틀릿
	case 1015:                          // 부적
	case 1016:                          // 반지
	case 1017:                          // 메달
	case 1018:                          // 구슬
	case 1019:                          // 핀
		info.width = 1; info.height = 1; break;
	}
	return info;
}

bool Inventory::CanPlace(int start_x, int start_y, int w, int h, long long exclude_item_uid)
{
	// 1. 범위 검사
	if (start_x < 0 || start_y < 0 || start_x + w > INV_MAX_COL || start_y + h > INV_MAX_ROW)
	{
		return false;
	}

	// 2. 충돌 검사
	for (int y = start_y; y < start_y + h; ++y)
	{
		for (int x = start_x; x < start_x + w; ++x) 
		{
			long long existing_uid = grid_[y][x];
			// 빈칸(0)이 아닌데, 내가 이동하려는 그 아이템(exclude_uid)도 아니라면 -> 충돌
			if (existing_uid != 0 && existing_uid != exclude_item_uid)
			{
				return false;
			}
		}
	}
	return true;
}

bool Inventory::PlaceItem(Item* item, int x, int y, bool rotated)
{
	if (!item) return false;

	ItemTemplate info = GetItemTemplate(item->template_id);
	int w = rotated ? info.height : info.width;
	int h = rotated ? info.width : info.height;

	// 놓을 수 있는지 검사
	if (!CanPlace(x, y, w, h, item->item_uid))
	{
		return false;
	}

	// 기존 위치 지우기 (만약 이미 인벤토리에 있던 아이템이라면)
	if (items_.find(item->item_uid) != items_.end())
	{
		Item* old_item = items_[item->item_uid];
		ItemTemplate old_info = GetItemTemplate(old_item->template_id);
		int old_w = old_item->is_rotated ? old_info.height : old_info.width;
		int old_h = old_item->is_rotated ? old_info.width : old_info.height;

		for (int r = old_item->y; r < old_item->y + old_h; ++r)
		{
			for (int c = old_item->x; c < old_item->x + old_w; ++c) 
			{
				if (grid_[r][c] == item->item_uid) grid_[r][c] = 0;
			}
		}
	}

	// 새 위치에 기록
	item->x = x;
	item->y = y;
	item->is_rotated = rotated;

	for (int r = y; r < y + h; ++r)
	{
		for (int c = x; c < x + w; ++c)
		{
			grid_[r][c] = item->item_uid;
		}
	}

	// 맵에 등록 (이미 있으면 덮어쓰기지만 포인터는 같음)
	items_[item->item_uid] = item;
	return true;
}

bool Inventory::AddItem(Item* item)
{
	ItemTemplate info = GetItemTemplate(item->template_id);
	
	// 빈 공간 완전 탐색 (좌상단부터)
	for (int y = 0; y < INV_MAX_ROW; ++y) 
	{
		for (int x = 0; x < INV_MAX_COL; ++x) 
		{
			// 기본 방향으로 시도
			if (CanPlace(x, y, info.width, info.height)) 
			{
				return PlaceItem(item, x, y, false);
			}
		}
	}
	return false; // 공간 부족
}

bool Inventory::MoveItem(long long item_uid, int new_x, int new_y, bool new_rotated)
{
	auto it = items_.find(item_uid);
	if (it == items_.end()) return false; // 없는 아이템

	Item* item = it->second;
	return PlaceItem(item, new_x, new_y, new_rotated);
}

void Inventory::ApplySortResult(const std::vector<std::tuple<long long, short, short, bool>>& slots)
{
	// 1. 그리드 전체 초기화
	for (int y = 0; y < INV_MAX_ROW; ++y)
	{
		for (int x = 0; x < INV_MAX_COL; ++x)
		{
			grid_[y][x] = 0;
		}
	}
	// 2. 클라이언트가 보낸 위치로 각 아이템을 순서대로 재배치
	for (const auto& s : slots)
	{
		long long uid  = std::get<0>(s);
		short     nx   = std::get<1>(s);
		short     ny   = std::get<2>(s);
		bool      rot  = std::get<3>(s);

		auto it = items_.find(uid);
		if (it == items_.end()) continue;

		Item* item = it->second;
		ItemTemplate info = GetItemTemplate(item->template_id);
		int w = rot ? info.height : info.width;
		int h = rot ? info.width  : info.height;

		// 범위 검사
		if (nx < 0 || ny < 0 || nx + w > INV_MAX_COL || ny + h > INV_MAX_ROW) continue;

		// 그리드에 기록
		item->x          = nx;
		item->y          = ny;
		item->is_rotated = rot;
		for (int r = ny; r < ny + h; ++r)
		{
			for (int c = nx; c < nx + w; ++c)
			{
				grid_[r][c] = uid;
			}
		}
	}
}

Item* Inventory::RemoveItem(long long item_uid)
{
	auto it = items_.find(item_uid);
	if (it == items_.end()) return nullptr;

	Item* item = it->second;

	ItemTemplate info = GetItemTemplate(item->template_id);
	int w = item->is_rotated ? info.height : info.width;
	int h = item->is_rotated ? info.width : info.height;

	// 그리드 비우기
	for (int y = item->y; y < item->y + h; ++y) 
	{
		for (int x = item->x; x < item->x + w; ++x)
		{
			if (grid_[y][x] == item_uid) grid_[y][x] = 0;
		}
	}

	items_.erase(it);
	return item;
}

Item* Inventory::FindItem(long long item_uid)
{
	auto it = items_.find(item_uid);
	if (it == items_.end()) return nullptr;
	return it->second;
}

std::vector<Item*> Inventory::GetAllItems() const
{
	std::vector<Item*> result;
	result.reserve(items_.size());
	for (const auto& pair : items_)
	{
		result.push_back(pair.second);
	}
	return result;
}

std::vector<std::pair<std::string, std::string>> Inventory::GetInventoryDataForRedis()
{
	std::vector<std::pair<std::string, std::string>> data;
	data.reserve(items_.size());

	for (const auto& pair : items_) 
	{
		Item* item = pair.second;
		// Format: "TemplateID:X:Y:Rotated"
		std::string val = std::to_string(item->template_id) + ":" +
			std::to_string(item->x) + ":" +
			std::to_string(item->y) + ":" +
			std::to_string(item->is_rotated);

		data.push_back({ std::to_string(item->item_uid), val });
	}
	return data;
}

void Inventory::PrintGrid()
{
	std::cout << "===== Inventory Grid =====" << std::endl;
	for (int y = 0; y < INV_MAX_ROW; ++y) 
	{
		for (int x = 0; x < INV_MAX_COL; ++x) 
		{
			if (grid_[y][x] == 0) std::cout << ". ";
			else std::cout << grid_[y][x] % 10 << " "; // ID 끝자리만 출력
		}
		std::cout << std::endl;
	}
}
