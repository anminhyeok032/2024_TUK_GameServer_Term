#include "InventoryUI.h"
#include <iostream>

// 전역 패킷 전송 함수 (외부 client.cpp 등에 정의된 것 사용)
extern void send_packet(void* packet);

InventoryUI g_inventoryUI;

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

	// [테스트] 더미 아이템 추가
	AddItem(1, 1001, 1, 0, 0, false);
	AddItem(2, 1002, 1, 5, 5, false);
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
	if (tid == 1001) return { 2, 3 }; // 대검
	if (tid == 1002) return { 2, 2 }; // 방패
	return { 1, 1 };
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

void InventoryUI::AddItem(long long uid, int tid, int cnt, int x, int y, bool rot)
{
	ClientItem item;
	item.item_uid = uid;
	item.template_id = tid;
	item.count = cnt;
	item.x = x;
	item.y = y;
	item.is_rotated = rot;
	// item.sprite 설정은 나중에 텍스처 로드 후
	myItems_.push_back(item);
}

void InventoryUI::SyncToServer()
{
	if (!isDirty_) return;

	//std::cout << "[Sync] Inventory syncing to server..." << std::endl;

	for (const auto& item : myItems_) {
		CS_ITEM_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_ITEM_MOVE;
		p.item_uid = item.item_uid;
		p.new_x = item.x;
		p.new_y = item.y;
		p.is_rotated = item.is_rotated;
		send_packet(&p);
	}

	isDirty_ = false;
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
			float itemY = (float)(UI_Y + 35 + item.y * SLOT_SIZE);

			ClientItemTemplate info = GetItemTemplate(item.template_id);
			int w = item.is_rotated ? info.h : info.w;
			int h = item.is_rotated ? info.w : info.h;

			sf::FloatRect bounds(itemX, itemY, (float)(w * SLOT_SIZE), (float)(h * SLOT_SIZE));

			if (bounds.contains((float)mousePos.x, (float)mousePos.y)) {
				isDragging_ = true;
				draggingItemUID_ = item.item_uid;
				dragOffset_ = sf::Vector2f(itemX - mousePos.x, itemY - mousePos.y);
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
				int gridY = (int)((dropY - UI_Y - 35 + (SLOT_SIZE / 2)) / SLOT_SIZE);

				ClientItemTemplate info = GetItemTemplate(item->template_id);
				int w = item->is_rotated ? info.h : info.w;
				int h = item->is_rotated ? info.w : info.h;

				if (CanPlace(gridX, gridY, w, h, item->item_uid)) {
					item->x = gridX;
					item->y = gridY;
					isDirty_ = true;
				}
				else {
					std::cout << "Can't place item there!" << std::endl;
				}
			}
			isDragging_ = false;
			draggingItemUID_ = -1;
		}
	}
	// 3. 우클릭 회전
	else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
		if (!isDragging_) {
			for (auto& item : myItems_) {
				float itemX = (float)(UI_X + 10 + item.x * SLOT_SIZE);
				float itemY = (float)(UI_Y + 35 + item.y * SLOT_SIZE);
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
	float bgHeight = INV_MAX_ROW * SLOT_SIZE + 40.f;

	sf::RectangleShape bg(sf::Vector2f(bgWidth, bgHeight));
	bg.setFillColor(sf::Color(50, 50, 50, 230));
	bg.setOutlineColor(sf::Color::White);
	bg.setOutlineThickness(2);
	bg.setPosition((float)UI_X, (float)UI_Y);
	window_->draw(bg);

	// 제목
	sf::Text title("INVENTORY (I:Close, Drag:Move, R-Click:Rotate)", *font_, 15);
	title.setFillColor(sf::Color::White);
	title.setPosition((float)UI_X + 10.f, (float)UI_Y + 5.f);
	window_->draw(title);

	// 그리드
	sf::RectangleShape slot(sf::Vector2f(SLOT_SIZE - MARGIN * 2.f, SLOT_SIZE - MARGIN * 2.f));
	slot.setFillColor(sf::Color(100, 100, 100, 150));
	slot.setOutlineColor(sf::Color::Black);
	slot.setOutlineThickness(1);

	for (int y = 0; y < INV_MAX_ROW; ++y) {
		for (int x = 0; x < INV_MAX_COL; ++x) {
			float slotX = (float)(UI_X + 10 + x * SLOT_SIZE);
			float slotY = (float)(UI_Y + 35 + y * SLOT_SIZE);
			slot.setPosition(slotX + MARGIN, slotY + MARGIN);
			window_->draw(slot);
		}
	}

	// 아이템 그리기
	sf::RectangleShape itemBox;
	itemBox.setFillColor(sf::Color::Red);
	itemBox.setOutlineColor(sf::Color::Yellow);
	itemBox.setOutlineThickness(1);

	for (const auto& item : myItems_) {
		if (isDragging_ && item.item_uid == draggingItemUID_) continue;

		float itemX = (float)(UI_X + 10 + item.x * SLOT_SIZE);
		float itemY = (float)(UI_Y + 35 + item.y * SLOT_SIZE);

		ClientItemTemplate info = GetItemTemplate(item.template_id);
		int w = item.is_rotated ? info.h : info.w;
		int h = item.is_rotated ? info.w : info.h;

		itemBox.setSize(sf::Vector2f(w * SLOT_SIZE - MARGIN * 2.f, h * SLOT_SIZE - MARGIN * 2.f));
		itemBox.setPosition(itemX + MARGIN, itemY + MARGIN);
		window_->draw(itemBox);
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

			itemBox.setFillColor(sf::Color(255, 0, 0, 180));
			itemBox.setSize(sf::Vector2f(w * SLOT_SIZE - MARGIN * 2.f, h * SLOT_SIZE - MARGIN * 2.f));
			itemBox.setPosition(drawX + MARGIN, drawY + MARGIN);
			window_->draw(itemBox);
		}
	}
}
