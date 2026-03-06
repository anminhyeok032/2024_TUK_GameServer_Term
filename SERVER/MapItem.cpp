#include "MapItem.h"
#include "protocol.h"
#include "Player.h" 

void MapItem::SendAddObjectPacket(int c_id)
{
	// c_id는 이 패킷을 받을 플레이어 ID
	if (!objects[c_id]) return;
	Player* p = dynamic_cast<Player*>(objects[c_id].get());
	if (!p) return;

	SC_ADD_MAP_ITEM_PACKET packet;
	packet.size = sizeof(SC_ADD_MAP_ITEM_PACKET);
	packet.type = SC_ADD_MAP_ITEM;
	packet.object_id = id_;
	packet.item_uid = item_uid;
	packet.template_id = template_id;
	packet.count = count;
	packet.x = x_;
	packet.y = y_;
	
	p->DoSend(&packet);
}

void MapItem::SendRemoveObjectPacket(int c_id)
{
	if (!objects[c_id]) return;
	Player* p = dynamic_cast<Player*>(objects[c_id].get());
	if (!p) return;

	// Player의 view_list_에서 이 MapItem 제거
	p->mut_view_.lock();
	p->view_list_.erase(id_);
	p->mut_view_.unlock();

	SC_REMOVE_MAP_ITEM_PACKET packet;
	packet.size = sizeof(SC_REMOVE_MAP_ITEM_PACKET);
	packet.type = SC_REMOVE_MAP_ITEM;
	packet.object_id = id_;
	
	p->DoSend(&packet);
}
