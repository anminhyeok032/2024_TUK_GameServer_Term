#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>

// [ItemSpriteSheet]
// 스프라이트시트 텍스처 1장을 로드하고,
// JSON에서 파싱한 id -> IntRect 매핑을 보유
// GetSprite()로 id에 해당하는 sf::Sprite를 반환

struct SpriteRect {
	int x, y, w, h;
};

class ItemSpriteSheet
{
private:
	sf::Texture texture_;

	// "Bone_0" -> SpriteRect
	std::unordered_map<std::string, SpriteRect> rects_;

	// 싱글턴 - 생성자/복사 금지
	ItemSpriteSheet() = default;
	ItemSpriteSheet(const ItemSpriteSheet&) = delete;
	ItemSpriteSheet& operator=(const ItemSpriteSheet&) = delete;

public:
	static ItemSpriteSheet& GetInstance();

	// 텍스처 + JSON 로드
	// texPath : "Resources/Textures/Bone.png"
	// jsonPath: "Resources/Data/Bone.json"
	// 반환값 : 로드 성공 여부
	bool Load(const std::string& texPath, const std::string& jsonPath);

	// id에 해당하는 Sprite 반환 (텍스처 포인터 포함)
	// 존재하지 않는 id일 경우 빈 Sprite 반환
	sf::Sprite GetSprite(const std::string& spriteId) const;

	// 등록된 SpriteRect 직접 조회 (스케일 계산 등에 활용)
	// 존재하지 않으면 nullptr 반환
	const SpriteRect* GetRect(const std::string& spriteId) const;

	// 로드 성공 여부
	bool IsLoaded() const { return !rects_.empty(); }
};