#pragma once
#include <Windows.h>
#include <vector>
#include <diesel.h>

enum SettingsType {
	ST_FLOAT,
	ST_INT,
	ST_UINT,
	ST_VEC2,
	ST_VEC3,
	ST_VEC4,
	ST_COLOR,
	ST_NONE
};

struct Token {

	enum TokenType { EMPTY, NUMBER, NAME, DELIMITER, OPEN_BRACES, CLOSE_BRACES, ASSIGN };

	Token() {}
	Token(TokenType type) : type(type) {}
	Token(TokenType type, float v) : type(type), value(v) {}
	Token(TokenType type, int i, int s) : type(type), index(i), size(s) {}

	TokenType type;
	float value;
	int index;
	int size;
};
// -------------------------------------------------------
// settings item
// -------------------------------------------------------
struct SettingsItem {
	uint32_t prefixHash;
	uint32_t hash;
	SettingsType type;
	int nameIndex;
	int length;
	union {
		int* iPtr;
		uint32_t* uiPtr;
		float* fPtr;
		ds::vec2* v2Ptr;
		ds::vec3* v3Ptr;
		ds::vec4* v4Ptr;
		ds::Color* cPtr;		
	} ptr;
};

class GameSettings {

public:
	GameSettings(const char* fileName) : _fileName(fileName) {}
	void add(const char* name, int* value, int defaultValue = 0);
	void add(const char* name, uint32_t* value, uint32_t defaultValue = 0);
	void add(const char* name, float* value, float defaultValue = 0.0f);
	void add(const char* name, ds::vec2* value, const ds::vec2& defaultValue = ds::vec2(0.0f));
	void add(const char* name, ds::vec3* value, const ds::vec3& defaultValue = ds::vec3(0.0f));
	void add(const char* name, ds::vec4* value, const ds::vec4& defaultValue = ds::vec4(0.0f));
	void add(const char* name, ds::Color* value, const ds::Color& defaultValue = ds::Color(255,255,255,255));
	void load();
	void save(const char* fileName);
	const bool contains(const char* name) const;
	const bool contains(const char* name, SettingsType type) const;
	const SettingsItem& get(const char* name) const;
	const size_t num() const {
		return _items.size();
	}
private:
	size_t find(const char* name) const;
	void setValue(const char* name, int nameIndex, int length, float* values, int count);
	const char* _fileName;
	std::vector<SettingsItem> _items;
	std::vector<Token> _tokens;
	char* _text;
	FILETIME _filetime;
};

const uint32_t FNV_Prime = 0x01000193; //   16777619
const uint32_t FNV_Seed = 0x811C9DC5; // 2166136261

inline uint32_t fnv1a(const char* text, uint32_t hash = FNV_Seed) {
	const unsigned char* ptr = (const unsigned char*)text;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * FNV_Prime;
	}
	return hash;
}

void GameSettings::add(const char* name, int* value, int defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_INT;
	item.ptr.iPtr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, uint32_t* value, uint32_t defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_UINT;
	item.ptr.uiPtr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, float* value, float defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_FLOAT;
	item.ptr.fPtr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, ds::vec2* value, const ds::vec2& defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_VEC2;
	item.ptr.v2Ptr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, ds::vec3* value, const ds::vec3& defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_VEC3;
	item.ptr.v3Ptr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, ds::vec4* value, const ds::vec4& defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_VEC4;
	item.ptr.v4Ptr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, ds::Color* value, const ds::Color& defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.hash = fnv1a(name);
	item.type = ST_COLOR;
	item.ptr.cPtr = value;
	_items.push_back(item);
}

void GameSettings::save(const char* fileName) {
	char name[128];
	FILE* fp = fopen(fileName, "w");
	if (fp) {
		for (size_t i = 0; i < _items.size(); ++i) {
			const SettingsItem& item = _items[i];
			strncpy(name, _text + item.nameIndex, item.length);
			name[item.length] = '\0';
			fprintf(fp, "%s = ", name);
			switch (item.type) {
				case ST_INT: fprintf(fp, "%d\n", *item.ptr.iPtr); break;
				case ST_UINT: fprintf(fp, "%d\n", *item.ptr.uiPtr); break;
				case ST_FLOAT: fprintf(fp, "%g\n", *item.ptr.fPtr); break;
				case ST_VEC2 : fprintf(fp, "%g, %g\n", item.ptr.v2Ptr->x, item.ptr.v2Ptr->y); break;
				case ST_VEC3: fprintf(fp, "%g, %g, %g\n", item.ptr.v3Ptr->x, item.ptr.v3Ptr->y, item.ptr.v3Ptr->z); break;
				case ST_VEC4: fprintf(fp, "%g, %g, %g, %g\n", item.ptr.v4Ptr->x, item.ptr.v4Ptr->y, item.ptr.v4Ptr->z, item.ptr.v4Ptr->w); break;
				case ST_COLOR: fprintf(fp, "%d, %d, %d, %d\n", (item.ptr.cPtr->r * 255.0f), (item.ptr.cPtr->g * 255.0f), (item.ptr.cPtr->b * 255.0f), (item.ptr.cPtr->a * 255.0f)); break;
			}
		}
		fclose(fp);
	}
}

char* loadFile(const char* fileName, FILETIME* time) {
	int size = 0;
	FILE *fp = fopen(fileName, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* buffer = new char[sz + 1];
		fread(buffer, 1, sz, fp);
		buffer[sz] = '\0';
		fclose(fp);
		getFileTime(fileName, time);
		return buffer;
	}
	return 0;
}

bool isDigit(const char c) {
	return ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.');
}

bool isNumeric(const char c) {
	return ((c >= '0' && c <= '9'));
}

bool isWhitespace(const char c) {
	if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
		return true;
	}
	return false;
}

float mystrtof(const char* p, char** endPtr) {
	while (isWhitespace(*p)) {
		++p;
	}
	float sign = 1.0f;
	if (*p == '-') {
		sign = -1.0f;
		++p;
	}
	else if (*p == '+') {
		++p;
	}
	float value = 0.0f;
	while (isNumeric(*p)) {
		value *= 10.0f;
		value = value + (*p - '0');
		++p;
	}
	if (*p == '.') {
		++p;
		float dec = 1.0f;
		float frac = 0.0f;
		while (isNumeric(*p)) {
			frac *= 10.0f;
			frac = frac + (*p - '0');
			dec *= 10.0f;
			++p;
		}
		value = value + (frac / dec);
	}
	if (endPtr) {
		*endPtr = (char *)(p);
	}
	return value * sign;
}

size_t GameSettings::find(const char* name) const {
	uint32_t hash = fnv1a(name);
	for (size_t i = 0; i < _items.size(); ++i) {
		if (_items[i].hash == hash) {
			return i;
		}
	}
	return -1;
}

void GameSettings::setValue(const char* name, int nameIndex, int length, float* values, int count) {
	size_t idx = find(name);
	if (idx != -1) {
		SettingsItem& item = _items[idx];
		item.nameIndex = nameIndex;
		item.length = length;
		if (item.type == ST_INT && count == 1) {
			*item.ptr.iPtr = values[0];
		}
		else if (item.type == ST_UINT && count == 1) {
			*item.ptr.uiPtr = values[0];
		}
		else if (item.type == ST_FLOAT && count == 1) {
			*item.ptr.fPtr = values[0];
		}
		else if (item.type == ST_VEC2 && count == 2) {
			item.ptr.v2Ptr->x = values[0];
			item.ptr.v2Ptr->y = values[1];
		}
		else if (item.type == ST_VEC3 && count == 3) {
			item.ptr.v3Ptr->x = values[0];
			item.ptr.v3Ptr->y = values[1];
			item.ptr.v3Ptr->z = values[2];
		}
		else if (item.type == ST_VEC4 && count == 4) {
			item.ptr.v4Ptr->x = values[0];
			item.ptr.v4Ptr->y = values[1];
			item.ptr.v4Ptr->z = values[2];
			item.ptr.v4Ptr->w = values[3];
		}
		else if (item.type == ST_COLOR && count == 4) {
			item.ptr.cPtr->r = values[0] / 255.0f;
			item.ptr.cPtr->g = values[1] / 255.0f;
			item.ptr.cPtr->b = values[2] / 255.0f;
			item.ptr.cPtr->a = values[3] / 255.0f;
		}
	}
}

void getFileTime(const char* fileName, FILETIME* time) {
	WORD ret = -1;
	// no file sharing mode
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		// Retrieve the file times for the file.
		GetFileTime(hFile, NULL, NULL, time);
		CloseHandle(hFile);
	}
}

void GameSettings::load() {
	int cnt = 0;
	_text = loadFile(_fileName, &_filetime);
	const char* p = _text;
	while (*p != 0) {
		Token token(Token::EMPTY);
		if (isDigit(*p)) {
			char *out;
			token = Token(Token::NUMBER, mystrtof(p, &out));
			p = out;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '.')) {
			const char *identifier = p;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p == '.') )
				p++;
			token = Token(Token::NAME, identifier - _text, p - identifier);
		}
		else if (*p == '#') {
			++p;
			while (*p != '\n') {
				++p;
				if (*p == 0) {
					break;
				}
			}
		}
		else {
			switch (*p) {
			case '{': token = Token(Token::OPEN_BRACES); break;
			case '}': token = Token(Token::CLOSE_BRACES); break;
			case ' ': case '\t': case '\n': case '\r': break;
			case '=': token = Token(Token::ASSIGN); break;
			case ',': token = Token(Token::DELIMITER); break;
			}
			++p;
		}
		if (token.type != Token::EMPTY) {
			_tokens.push_back(token);
		}
	}
	int idx = 0;
	Token& t = _tokens[idx];
	char name[128];
	float values[128];
	while (idx < _tokens.size()) {
		if (t.type == Token::NAME) {
			strncpy(name, _text + t.index, t.size);
			name[t.size] = '\0';
			++idx;
			Token& n = _tokens[idx];
			if (n.type == Token::ASSIGN) {
				++idx;
				Token& v = _tokens[idx];
				int count = 0;
				while (v.type == Token::NUMBER || v.type == Token::DELIMITER) {
					if (v.type == Token::NUMBER) {
						if (count < 128) {
							values[count++] = v.value;
						}
					}
					++idx;
					if (idx >= _tokens.size()) {
						break;
					}
					v = _tokens[idx];
				}
				setValue(name, t.index, t.size, values, count);
			}
		}
		else {
			++idx;
		}
		if (idx < _tokens.size()) {
			t = _tokens[idx];
		}
	}
}
