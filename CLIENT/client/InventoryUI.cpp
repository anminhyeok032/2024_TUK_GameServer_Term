#include "InventoryUI.h"
#include "ItemSpriteSheet.h"
#include <iostream>

// 전역 패킷 전송 함수 (외부 client.cpp 등에 정의된 것 사용)
extern void send_packet(void* packet);

InventoryUI g_inventoryUI;

static void DrawItemSprite(sf::RenderWindow* window,
	const std::string& sprite_id,
	float slotX, float slotY,
	float drawW, float drawH,
	bool is_rotated,
	sf::Uint8 alpha = 255)
{
	sf::Sprite sprite = ItemSpriteSheet::GetInstance().GetSprite(sprite_id);
	const SpriteRect* r = ItemSpriteSheet::GetInstance().GetRect(sprite_id);
	if (!r || r->w <= 0 || r->h <= 0) return;

	sprite.setColor(sf::Color(255, 255, 255, alpha));

	if (!is_rotated)
	{
		// 슬롯 크기에 맞게 스케일 조정
		sprite.setScale(drawW / r->w, drawH / r->h);
		sprite.setPosition(slotX, slotY);
	}
	else
	{
		// 90° CW 회전 시:
		//   화면상 가로 = 원본 세로 * scaleY = drawW
		//   화면상 세로 = 원본 가로 * scaleX = drawH
		float sx = drawH / r->w;
		float sy = drawW / r->h;
		sprite.setScale(sx, sy);
		sprite.setRotation(90.f);
		// 위치 보정: 회전 후 좌상단이 슬롯 좌상단에 오도록
		sprite.setPosition(slotX + drawW, slotY);
	}

	window->draw(sprite);
}

InventoryUI::InventoryUI()
	: window_(nullptr), font_(nullptr), isActive_(false), isDragging_(false), draggingItemUID_(-1), isDirty_(false)
{
	lastSyncTime_ = std::chrono::system_clock::now();
}

InventoryUI::~InventoryUI()
{
	myItems_.clear();
}

void InventoryUI::Initialize(sf::RenderWindow* window, sf::Font* font)
{
	window_ = window;
	font_ = font;
	// 아이템은 서버로부터 SC_GET_ITEM 패킷을 받아 AddItem()으로 추가됨
}

void InventoryUI::Toggle()
{
	isActive_ = !isActive_;
	// 닫을 때 변경사항 있으면 즉시 동기화
	if (!isActive_ && isDirty_) {
		SyncToServer();
	}
}

ClientItemTemplate InventoryUI::GetItemTemplate(int tid)
{
	const ItemInfo* info = ItemDatabase::GetInstance().Get(tid);
	if (!info) return { 1, 1, "", "" };
	return { info->grid_w, info->grid_h, info->sprite_id, info->name };
}

ClientItem* InventoryUI::FindItem(long long uid)
{
	for (auto& item : myItems_) {
		if (item.item_uid == uid) return &item;
	}
	return nullptr;
}

bool InventoryUI::CanPlace(int startX, int startY, int w, int h, long long excludeUID)
{
	if (startX < 0 || startY < 0 || startX + w > INV_MAX_COL || startY + h > INV_MAX_ROW)
		return false;

	for (const auto& item : myItems_) {
		if (item.item_uid == excludeUID) continue;

		ClientItemTemplate info = GetItemTemplate(item.template_id);
		int itemW = item.is_rotated ? info.h : info.w;
		int itemH = item.is_rotated ? info.w : info.h;

		// AABB 충돌 검사
		if (startX < item.x + itemW && startX + w > item.x &&
			startY < item.y + itemH && startY + h > item.y) {
			return false; // 겹침
		}
	}
	return true;
}

bool InventoryUI::HasSpaceFor(int template_id)
{
	ClientItemTemplate info = GetItemTemplate(template_id);
	// 서버 Inventory::AddItem과 동일한 탐색 순서 (좌상단부터)
	for (int y = 0; y < INV_MAX_ROW; ++y)
	{
		for (int x = 0; x < INV_MAX_COL; ++x)
		{
			if (CanPlace(x, y, info.w, info.h, -1))
				return true;
		}
	}
	return false;
}

void InventoryUI::SortItems()
{
	if (myItems_.empty()) return;

	// 1. 면적 내림차순 정렬 (크기 같으면 w 내림차순)
	std::vector<ClientItem*> sorted;
	for (auto& item : myItems_)
	{
		sorted.push_back(&item);
	}

	std::sort(sorted.begin(), sorted.end(), [&](const ClientItem* a, const ClientItem* b) {
		ClientItemTemplate ia = GetItemTemplate(a->template_id);
		ClientItemTemplate ib = GetItemTemplate(b->template_id);
		int areaA = ia.w * ia.h;
		int areaB = ib.w * ib.h;
		if (areaA != areaB) return areaA > areaB;
		return ia.w > ib.w;
	});

	// 2. 모든 아이템 rotated = false 초기화 (정렬 기준은 회전 없이 계산)
	for (auto& item : myItems_)
	{
		item.is_rotated = false;
	}

	int grid[INV_MAX_ROW][INV_MAX_COL] = {};

	auto canPlaceOnGrid = [&](int sx, int sy, int w, int h) {
		if (sx < 0 || sy < 0 || sx + w > INV_MAX_COL || sy + h > INV_MAX_ROW)	return false;
		for (int gy = sy; gy < sy + h; ++gy)
		{
			for (int gx = sx; gx < sx + w; ++gx)
			{
				if (grid[gy][gx] != 0) return false;
			}
		}
		return true;
	};

	auto placeOnGrid = [&](int sx, int sy, int w, int h) {
		for (int gy = sy; gy < sy + h; ++gy)
		{
			for (int gx = sx; gx < sx + w; ++gx)
			{
				grid[gy][gx] = 1;
			}
		}
	};

	for (ClientItem* item : sorted)
	{
		ClientItemTemplate info = GetItemTemplate(item->template_id);
		int w = info.w;
		int h = info.h;
		bool placed = false;
		for (int y = 0; y < INV_MAX_ROW && !placed; ++y) 
		{
			for (int x = 0; x < INV_MAX_COL && !placed; ++x) 
			{
				if (canPlaceOnGrid(x, y, w, h)) 
				{
					placeOnGrid(x, y, w, h);
					item->x = x;
					item->y = y;
					placed = true;
				}
			}
		}
	}

	// 4. 정렬된 전체 아이템 위치를 CS_ITEM_SORT_PACKET 1개로 서버에 전송
	int count = (int)myItems_.size();
	int packetSize = (int)(sizeof(CS_ITEM_SORT_PACKET) + sizeof(SortSlot) * (count - 1));
	std::vector<char> buf(packetSize, 0);
	CS_ITEM_SORT_PACKET* pkt = reinterpret_cast<CS_ITEM_SORT_PACKET*>(buf.data());
	pkt->size = (unsigned short)packetSize;
	pkt->type = CS_ITEM_SORT;
	pkt->item_count = count;
	for (int i = 0; i < count; ++i)
	{
		pkt->slots[i].item_uid   = myItems_[i].item_uid;
		pkt->slots[i].x          = (short)myItems_[i].x;
		pkt->slots[i].y          = (short)myItems_[i].y;
		pkt->slots[i].is_rotated = myItems_[i].is_rotated;
	}
	send_packet(pkt);

	// 5. Dirty Sync 초기화 (정렬로 이미 서버에 전송 완료)
	isDirty_ = false;
	dirtyItemUIDs_.clear();
	lastSyncTime_ = std::chrono::system_clock::now();

	push_status_message(sf::String(ToSfString("[Inventory] 정렬 성공.")));
}

void InventoryUI::AddItem(long long uid, int tid, int x, int y, bool rot)
{
	// 중복 UID 방지 - 이미 있으면 무시
	for (const auto& existing : myItems_) 
	{
		if (existing.item_uid == uid) return;
	}
	std::cout << "[InventoryUI] Adding item UID " << uid << " (template " << tid << ") at (" << x << "," << y << "), rotated: " << rot << std::endl;

	ClientItem item;
	item.item_uid = uid;
	item.template_id = tid;
	item.x = x;
	item.y = y;
	item.is_rotated = rot;
	myItems_.push_back(item);
}

void InventoryUI::RemoveItem(long long uid)
{
	for (auto it = myItems_.begin(); it != myItems_.end(); ++it)
	{
		if (it->item_uid == uid)
		{
			myItems_.erase(it);
			break;
		}
	}
}

void InventoryUI::SyncToServer()
{
	if (!isDirty_ || dirtyItemUIDs_.empty()) return;

	std::cout << "[Sync] Syncing " << dirtyItemUIDs_.size() << " changed items to server..." << std::endl;

	// 변경된 아이템만 골라서 패킷 전송
	for (long long uid : dirtyItemUIDs_) {
		ClientItem* item = FindItem(uid);
		if (!item) continue;

		CS_ITEM_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_ITEM_MOVE;
		p.item_uid = item->item_uid;
		p.new_x = item->x;
		p.new_y = item->y;
		p.is_rotated = item->is_rotated;
		send_packet(&p);
	}

	isDirty_ = false;
	dirtyItemUIDs_.clear(); // 목록 초기화
	lastSyncTime_ = std::chrono::system_clock::now();
}

void InventoryUI::UpdateSync()
{
	if (!isActive_) return;
	auto now = std::chrono::system_clock::now();
	if (now - lastSyncTime_ > std::chrono::seconds(5)) {
		SyncToServer();
	}
}

void InventoryUI::HandleInput(sf::Event& event)
{
	if (!isActive_ || !window_) return;

	sf::Vector2i mousePos = sf::Mouse::getPosition(*window_);

	// 1. 드래그 시작
	if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
		for (auto& item : myItems_) {
			float itemX = (float)(UI_X + 10 + item.x * SLOT_SIZE);
			float itemY = (float)(UI_Y + HEADER_H + item.y * SLOT_SIZE);

			ClientItemTemplate info = GetItemTemplate(item.template_id);
			int w = item.is_rotated ? info.h : info.w;
			int h = item.is_rotated ? info.w : info.h;

			sf::FloatRect bounds(itemX, itemY, (float)(w * SLOT_SIZE), (float)(h * SLOT_SIZE));

			if (bounds.contains((float)mousePos.x, (float)mousePos.y)) {
				isDragging_ = true;
				draggingItemUID_ = item.item_uid;
				dragOffset_ = sf::Vector2f(itemX - mousePos.x, itemY - mousePos.y);
				dragOrigX_ = item.x;
				dragOrigY_ = item.y;
				dragOrigRotated_ = item.is_rotated;
				break;
			}
		}
	}
	// 2. 드래그 종료
	else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
		if (isDragging_) {
			ClientItem* item = FindItem(draggingItemUID_);
			if (item) {
				float dropX = mousePos.x + dragOffset_.x;
				float dropY = mousePos.y + dragOffset_.y;

				// 그리드 좌표 변환 (+ 반올림 보정)
				int gridX = (int)((dropX - UI_X - 10 + (SLOT_SIZE / 2)) / SLOT_SIZE);
				int gridY = (int)((dropY - UI_Y - HEADER_H + (SLOT_SIZE / 2)) / SLOT_SIZE);

				ClientItemTemplate info = GetItemTemplate(item->template_id);
				int w = item->is_rotated ? info.h : info.w;
				int h = item->is_rotated ? info.w : info.h;

				// 인벤토리 영역 내에 있는지 검사
				if (CanPlace(gridX, gridY, w, h, item->item_uid)) {
					item->x = gridX;
					item->y = gridY;
					isDirty_ = true;
					dirtyItemUIDs_.insert(item->item_uid); // [Add] 변경된 아이템 UID 등록
				}
				else {
					// 인벤토리 영역 밖으로 드롭 -> 아이템 버리기
				sf::FloatRect invRect((float)UI_X, (float)UI_Y,
					(float)(INV_MAX_COL * SLOT_SIZE + 20), (float)(INV_MAX_ROW * SLOT_SIZE + HEADER_H));

					if (!invRect.contains((float)mousePos.x, (float)mousePos.y)) {
						// 버리기 패킷 전송
						CS_ITEM_DROP_PACKET p;
						p.size = sizeof(p);
						p.type = CS_ITEM_DROP;
						p.item_uid = draggingItemUID_;
						send_packet(&p);

						// 버리기 상태 메시지
						ClientItemTemplate dropInfo = GetItemTemplate(item->template_id);
						// ToSfString()으로 CP949 아이템 이름을 sf::String으로 변환
						sf::String dropItemName = dropInfo.name.empty()
							? sf::String("Unknown Item")
							: ToSfString(dropInfo.name);
						sf::String dropMsg = sf::String("[Item Drop] ") + dropItemName
							+ " x 1";
						push_status_message(dropMsg);

						// 클라이언트 인벤토리에서 즉시 제거
						RemoveItem(draggingItemUID_);
						// 버리기는 Dirty Sync 대상이 아니므로 즉시 처리됨
					}
					else {
						// placement failed inside inventory — restore original rotation
						item->is_rotated = dragOrigRotated_;
						push_status_message(sf::String(ToSfString("[Inventory] 아이템을 놓을 수 없습니다.")));
						//std::cout << "Can't place item there!" << std::endl;
					}
				}
			}
			isDragging_ = false;
			draggingItemUID_ = -1;
		}
	}
	// 3. 우클릭 회전
	else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
		if (isDragging_) {
			// 드래그 중: 들고 있는 아이템 회전 (놓을 때 CanPlace로 검증)
			ClientItem* item = FindItem(draggingItemUID_);
			if (item) {
				item->is_rotated = !item->is_rotated;
			}
		}
		else {
			for (auto& item : myItems_) {
				float itemX = (float)(UI_X + 10 + item.x * SLOT_SIZE);
				float itemY = (float)(UI_Y + HEADER_H + item.y * SLOT_SIZE);
				ClientItemTemplate info = GetItemTemplate(item.template_id);
				int w = item.is_rotated ? info.h : info.w;
				int h = item.is_rotated ? info.w : info.h;
				sf::FloatRect bounds(itemX, itemY, (float)(w * SLOT_SIZE), (float)(h * SLOT_SIZE));

				if (bounds.contains((float)mousePos.x, (float)mousePos.y)) {
					bool nextRotated = !item.is_rotated;
					int nextW = nextRotated ? info.h : info.w;
					int nextH = nextRotated ? info.w : info.h;

					if (CanPlace(item.x, item.y, nextW, nextH, item.item_uid)) {
						item.is_rotated = nextRotated;
						isDirty_ = true;
						dirtyItemUIDs_.insert(item.item_uid); // 변경된 아이템 UID 등록
					}
					break;
				}
			}
		}
	}
}

void InventoryUI::Draw()
{
	if (!isActive_ || !window_) return;

	// 배경
	float bgWidth = INV_MAX_COL * SLOT_SIZE + 20.f;
	float bgHeight = INV_MAX_ROW * SLOT_SIZE + (float)HEADER_H;

	sf::RectangleShape bg(sf::Vector2f(bgWidth, bgHeight));
	bg.setFillColor(sf::Color(50, 50, 50, 230));
	bg.setOutlineColor(sf::Color::White);
	bg.setOutlineThickness(2);
	bg.setPosition((float)UI_X, (float)UI_Y);
	window_->draw(bg);

	// 제목
	sf::Text title("INVENTORY  I:Close  Drag:Move", *font_, 13);
	title.setFillColor(sf::Color::White);
	title.setPosition((float)UI_X + 10.f, (float)UI_Y + 3.f);
	window_->draw(title);

	sf::Text title2("R-Click:Rotate  O:Sort", *font_, 13);
	title2.setFillColor(sf::Color::White);
	title2.setPosition((float)UI_X + 10.f, (float)UI_Y + 18.f);
	window_->draw(title2);

	// 그리드 — 아이템이 차지하는 칸은 다른 색으로 표시
	// 1) 점유 맵 계산
	bool occupied[INV_MAX_ROW][INV_MAX_COL] = {};
	for (const auto& item : myItems_) {
		if (isDragging_ && item.item_uid == draggingItemUID_) continue;
		ClientItemTemplate ti = GetItemTemplate(item.template_id);
		int iw = item.is_rotated ? ti.h : ti.w;
		int ih = item.is_rotated ? ti.w : ti.h;
		for (int ry = item.y; ry < item.y + ih && ry < INV_MAX_ROW; ++ry)
			for (int rx = item.x; rx < item.x + iw && rx < INV_MAX_COL; ++rx)
				occupied[ry][rx] = true;
	}

	// 2) 슬롯 그리기 (빈칸 / 점유칸 색상 구분)
	sf::RectangleShape slot(sf::Vector2f(SLOT_SIZE - MARGIN * 2.f, SLOT_SIZE - MARGIN * 2.f));
	slot.setOutlineColor(sf::Color::Black);
	slot.setOutlineThickness(1);

	for (int y = 0; y < INV_MAX_ROW; ++y) {
		for (int x = 0; x < INV_MAX_COL; ++x) {
			float slotX = (float)(UI_X + 10 + x * SLOT_SIZE);
			float slotY = (float)(UI_Y + HEADER_H + y * SLOT_SIZE);
			if (occupied[y][x])
				slot.setFillColor(sf::Color(70, 90, 70, 180));
			else
				slot.setFillColor(sf::Color(100, 100, 100, 150));
			slot.setPosition(slotX + MARGIN, slotY + MARGIN);
			window_->draw(slot);
		}
	}

	// 아이템 그리기
	sf::RectangleShape itemBox;
	itemBox.setOutlineColor(sf::Color::Yellow);
	itemBox.setOutlineThickness(1);

	// 아이템 경계선용 사각형
	sf::RectangleShape itemBorder;
	itemBorder.setFillColor(sf::Color::Transparent);
	itemBorder.setOutlineColor(sf::Color(200, 200, 100, 200));
	itemBorder.setOutlineThickness(1.f);

	for (const auto& item : myItems_) {
		if (isDragging_ && item.item_uid == draggingItemUID_) continue;

		float itemX = (float)(UI_X + 10 + item.x * SLOT_SIZE);
		float itemY = (float)(UI_Y + HEADER_H + item.y * SLOT_SIZE);

		ClientItemTemplate info = GetItemTemplate(item.template_id);
		int w = item.is_rotated ? info.h : info.w;
		int h = item.is_rotated ? info.w : info.h;

		float drawW = w * SLOT_SIZE - MARGIN * 2.f;
		float drawH = h * SLOT_SIZE - MARGIN * 2.f;

		if (!info.sprite_id.empty() && ItemSpriteSheet::GetInstance().IsLoaded())
		{
			DrawItemSprite(window_,
				info.sprite_id,
				itemX + MARGIN, itemY + MARGIN,
				drawW, drawH,
				item.is_rotated);
		}
		else
		{
			itemBox.setFillColor(sf::Color(100, 100, 100, 200));
			itemBox.setSize(sf::Vector2f(drawW, drawH));
			itemBox.setPosition(itemX + MARGIN, itemY + MARGIN);
			window_->draw(itemBox);
		}

		// 아이템 경계선
		itemBorder.setSize(sf::Vector2f(drawW, drawH));
		itemBorder.setPosition(itemX + MARGIN, itemY + MARGIN);
		window_->draw(itemBorder);
	}

	// 드래그 중인 아이템
	if (isDragging_) {
		ClientItem* item = FindItem(draggingItemUID_);
		if (item) {
			sf::Vector2i mousePos = sf::Mouse::getPosition(*window_);
			float drawX = mousePos.x + dragOffset_.x;
			float drawY = mousePos.y + dragOffset_.y;

			ClientItemTemplate info = GetItemTemplate(item->template_id);
			int w = item->is_rotated ? info.h : info.w;
			int h = item->is_rotated ? info.w : info.h;

			float drawW = w * SLOT_SIZE - MARGIN * 2.f;
			float drawH = h * SLOT_SIZE - MARGIN * 2.f;

			if (!info.sprite_id.empty() && ItemSpriteSheet::GetInstance().IsLoaded())
			{
				// 드래그 중: 반투명 (alpha=180)
				DrawItemSprite(window_,
					info.sprite_id,
					drawX + MARGIN, drawY + MARGIN,
					drawW, drawH,
					item->is_rotated,
					180);
			}
			else
			{
				// fallback
				itemBox.setFillColor(sf::Color(100, 100, 100, 130));
				itemBox.setSize(sf::Vector2f(drawW, drawH));
				itemBox.setPosition(drawX + MARGIN, drawY + MARGIN);
				window_->draw(itemBox);
			}
		}
	}
}