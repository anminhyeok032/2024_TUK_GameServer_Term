#include "ItemSpriteSheet.h"
#include <fstream>
#include <sstream>
#include <iostream>

// ============================================================
// [내부 파싱 유틸리티]
// 외부 라이브러리 없이 고정 구조의 JSON을 파싱
// 지원 형식:
//   [ { "id": "...", "rect": { "x":N, "y":N, "width":N, "height":N } }, ... ]
// ============================================================

// pos 이후에서 key를 찾아 그 값(문자열)을 반환, pos를 값 끝으로 이동
// 예: "id": "Bone_0"  ->  "Bone_0" 반환
static bool ParseStringValue(const std::string& src, size_t& pos,
	const std::string& key, std::string& outVal)
{
	// "key" 위치 탐색
	std::string token = "\"" + key + "\"";
	size_t found = src.find(token, pos);
	if (found == std::string::npos) return false;

	// ':' 건너뜀
	size_t colon = src.find(':', found + token.size());
	if (colon == std::string::npos) return false;

	// 여는 따옴표 탐색
	size_t open = src.find('"', colon + 1);
	if (open == std::string::npos) return false;

	// 닫는 따옴표 탐색
	size_t close = src.find('"', open + 1);
	if (close == std::string::npos) return false;

	outVal = src.substr(open + 1, close - open - 1);
	pos = close + 1;
	return true;
}

// pos 이후에서 key를 찾아 그 값(정수)을 반환, pos를 값 끝으로 이동
// 예: "x": 32  ->  32 반환
static bool ParseIntValue(const std::string& src, size_t& pos,
	const std::string& key, int& outVal)
{
	std::string token = "\"" + key + "\"";
	size_t found = src.find(token, pos);
	if (found == std::string::npos) return false;

	size_t colon = src.find(':', found + token.size());
	if (colon == std::string::npos) return false;

	// 숫자 시작 위치 탐색 (공백/개행 건너뜀)
	size_t numStart = colon + 1;
	while (numStart < src.size() && (src[numStart] == ' ' || src[numStart] == '\n'
		|| src[numStart] == '\r' || src[numStart] == '\t'))
	{
		++numStart;
	}
	if (numStart >= src.size()) return false;

	// stoi로 파싱 (음수 포함)
	size_t numEnd = 0;
	outVal = std::stoi(src.substr(numStart), &numEnd);
	pos = numStart + numEnd;
	return true;
}

// ============================================================

ItemSpriteSheet& ItemSpriteSheet::GetInstance()
{
	static ItemSpriteSheet instance;
	return instance;
}

bool ItemSpriteSheet::Load(const std::string& texPath, const std::string& jsonPath)
{
	// 1. 텍스처 로드
	if (!texture_.loadFromFile(texPath))
	{
		std::cerr << "[ItemSpriteSheet] 텍스처 로드 실패: " << texPath << std::endl;
		return false;
	}

	// 2. JSON 파일 전체를 문자열로 읽기
	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		std::cerr << "[ItemSpriteSheet] JSON 파일 열기 실패: " << jsonPath << std::endl;
		return false;
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	const std::string src = ss.str();

	// 3. 항목별 파싱
	// 배열 원소 '{' ~ '}' 단위로 순회
	size_t pos = 0;
	while (true)
	{
		// 다음 '{' 탐색 (항목 시작)
		size_t blockStart = src.find('{', pos);
		if (blockStart == std::string::npos) break;

		// 대응하는 '}' 탐색 (중첩 고려)
		int depth = 0;
		size_t blockEnd = std::string::npos;
		for (size_t i = blockStart; i < src.size(); ++i)
		{
			if (src[i] == '{') ++depth;
			else if (src[i] == '}') { --depth; if (depth == 0) { blockEnd = i; break; } }
		}
		if (blockEnd == std::string::npos) break;

		// 항목 블록 추출
		std::string block = src.substr(blockStart, blockEnd - blockStart + 1);
		size_t bpos = 0;

		// "id" 파싱
		std::string id;
		if (!ParseStringValue(block, bpos, "id", id))
		{
			pos = blockEnd + 1;
			continue;
		}

		// "rect" 블록 내 정수 4개 파싱
		// bpos를 "rect" 위치로 이동 후 각 키 탐색
		size_t rectStart = block.find("\"rect\"", bpos);
		if (rectStart == std::string::npos) { pos = blockEnd + 1; continue; }
		bpos = rectStart;

		SpriteRect rect;
		if (!ParseIntValue(block, bpos, "x", rect.x) ||
			!ParseIntValue(block, bpos, "y", rect.y) ||
			!ParseIntValue(block, bpos, "width", rect.w) ||
			!ParseIntValue(block, bpos, "height", rect.h))
		{
			std::cerr << "[ItemSpriteSheet] rect 파싱 실패: " << id << std::endl;
			pos = blockEnd + 1;
			continue;
		}

		rects_[id] = rect;
		pos = blockEnd + 1;
	}

	if (rects_.empty())
	{
		std::cerr << "[ItemSpriteSheet] 파싱 결과 0개 — JSON 형식을 확인하세요." << std::endl;
		return false;
	}

	std::cout << "[ItemSpriteSheet] 로드 완료 - "
		<< rects_.size() << "개 스프라이트 (" << texPath << ")" << std::endl;

	return true;
}

sf::Sprite ItemSpriteSheet::GetSprite(const std::string& spriteId) const
{
	sf::Sprite sprite;

	auto it = rects_.find(spriteId);
	if (it == rects_.end())
	{
		std::cerr << "[ItemSpriteSheet] 존재하지 않는 id: " << spriteId << std::endl;
		return sprite;
	}

	const SpriteRect& r = it->second;
	sprite.setTexture(texture_);
	sprite.setTextureRect(sf::IntRect(r.x, r.y, r.w, r.h));
	return sprite;
}

const SpriteRect* ItemSpriteSheet::GetRect(const std::string& spriteId) const
{
	auto it = rects_.find(spriteId);
	if (it == rects_.end()) return nullptr;
	return &it->second;
}