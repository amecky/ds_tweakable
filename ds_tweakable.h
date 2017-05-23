#pragma once
#include <vector>
#include <diesel.h>

enum SettingsType {
	ST_FLOAT,
	ST_RECT,
	ST_INT,
	ST_VEC2,
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
	const char* name;
	SettingsType type;
	union {
		int* iPtr;
		float* fPtr;
		ds::vec2* v2Ptr;
		//Color* cPtr;
		//Rect* rPtr;
	} ptr;
};

class GameSettings {

public:
	GameSettings(const char* fileName) : _fileName(fileName) {}
	void add(const char* name, float* value, float defaultValue = 0.0f);
	void add(const char* name, ds::vec2* value, const ds::vec2& defaultValue = ds::vec2(0.0f));
	void load();
	void save();
	const bool contains(const char* name) const;
	const bool contains(const char* name, SettingsType type) const;
	const SettingsItem& get(const char* name) const;
	const size_t num() const {
		return _items.size();
	}
private:
	size_t find(const char* name) const;
	void setValue(const char* name, float* values, int count);
	const char* _fileName;
	std::vector<SettingsItem> _items;
	std::vector<Token> _tokens;
	char* _text;
};

void GameSettings::add(const char* name, float* value, float defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.name = name;
	item.type = ST_FLOAT;
	item.ptr.fPtr = value;
	_items.push_back(item);
}

void GameSettings::add(const char* name, ds::vec2* value, const ds::vec2& defaultValue) {
	*value = defaultValue;
	SettingsItem item;
	item.name = name;
	item.type = ST_VEC2;
	item.ptr.v2Ptr = value;
	_items.push_back(item);
}

void GameSettings::save() {
	for (size_t i = 0; i < _items.size(); ++i) {

	}
}

char* loadFile(const char* fileName) {
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
	for (size_t i = 0; i < _items.size(); ++i) {
		if (strcmp(_items[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

void GameSettings::setValue(const char* name, float* values, int count) {
	size_t idx = find(name);
	if (idx != -1) {
		SettingsItem& item = _items[idx];
		if (item.type == ST_FLOAT && count == 1) {
			*item.ptr.fPtr = values[0];
		}
		else if (item.type == ST_VEC2 && count == 2) {
			item.ptr.v2Ptr->x = values[0];
			item.ptr.v2Ptr->y = values[1];
		}
	}
}

void GameSettings::load() {
	int cnt = 0;
	_text = loadFile(_fileName);
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
	printf("tokens: %d\n", _tokens.size());
	int idx = 0;
	Token& t = _tokens[idx];
	char name[128];
	float values[128];
	while (idx < _tokens.size()) {
		if (t.type == Token::NAME) {
			strncpy(name, _text + t.index, t.size);
			name[t.size] = '\0';
			++idx;
			// build hash
			// get value index
			Token& n = _tokens[idx];
			if (n.type == Token::ASSIGN) {
				++idx;
				Token& v = _tokens[idx];
				int count = 0;
				while (v.type == Token::NUMBER || v.type == Token::DELIMITER) {
					if (v.type == Token::NUMBER) {
						// add value
						values[count++] = v.value;
					}
					++idx;
					if (idx >= _tokens.size()) {
						break;
					}
					v = _tokens[idx];
				}
				// FIXME: find item
				setValue(name, values, count);
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
