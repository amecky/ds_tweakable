#pragma once
#include <Windows.h>
#include <vector>
#include <diesel.h>

void twk_init(const char* fileName);

void twk_add(const char* category, const char* name, float* value);

void twk_add(const char* category, const char* name, ds::vec2* value);

void twk_add(const char* category, const char* name, ds::vec3* value);

void twk_add(const char* category, const char* name, ds::vec4* value);

void twk_add(const char* category, const char* name, float* array,int size);

void twk_shutdown();

bool twk_load();

void twk_save();

//#define GAMESETTINGS_IMPLEMENTATION

#ifdef GAMESETTINGS_IMPLEMENTATION


// -------------------------------------------------------
// game settings item
// -------------------------------------------------------
struct GameSettingsItem {

	enum SettingsType { ST_FLOAT, ST_INT, ST_UINT, ST_VEC2,	ST_VEC3, ST_VEC4, ST_COLOR,	ST_ARRAY, ST_NONE };

	uint32_t categoryHash;
	int categoryNameIndex;
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
		float* arPtr;
	} ptr;
	int arrayLength;
};

struct InternalCharBuffer {
	char* data;
	int capacity;
	int size;
	int next;
	int count;
	int* indices;
	int* sizes;
};

// -------------------------------------------------------
// internal settings context
// -------------------------------------------------------
struct SettingsContext {
	const char* fileName;
	FILETIME filetime;
	std::vector<GameSettingsItem> items;
	bool loaded;
	InternalCharBuffer charBuffer;
};

static SettingsContext* _settingsCtx = 0;

// -------------------------------------------------------
// init
// -------------------------------------------------------
void twk_init(const char* fileName) {
	_settingsCtx = new SettingsContext;
	_settingsCtx->fileName = fileName;
	_settingsCtx->loaded = false;
	_settingsCtx->charBuffer.data = 0;
	_settingsCtx->charBuffer.capacity = 0;
	_settingsCtx->charBuffer.count = 0;
	_settingsCtx->charBuffer.next = 0;
	_settingsCtx->charBuffer.indices = 0;
	_settingsCtx->charBuffer.sizes = 0;
	_settingsCtx->charBuffer.size = 0;
}

// -------------------------------------------------------
// shutdown
// -------------------------------------------------------
void twk_shutdown() {
	if (_settingsCtx != 0) {
		delete _settingsCtx;
	}
}

// -------------------------------------------------------
// Hashing
// -------------------------------------------------------
const uint32_t FNV_Prime = 0x01000193; 
const uint32_t FNV_Seed = 0x811C9DC5;

inline uint32_t twk_fnv1a(const char* text, uint32_t hash = FNV_Seed) {
	const unsigned char* ptr = (const unsigned char*)text;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * FNV_Prime;
	}
	return hash;
}

// -------------------------------------------------------
// internal add
// -------------------------------------------------------
static size_t twk_add(const char* category, const char* name, GameSettingsItem::SettingsType type) {
	GameSettingsItem item;
	item.categoryHash = twk_fnv1a(category);
	item.hash = twk_fnv1a(name);
	item.type = type;
	item.arrayLength = 0;
	_settingsCtx->items.push_back(item);
	return _settingsCtx->items.size() - 1;
}

// -------------------------------------------------------
// add float
// -------------------------------------------------------
void twk_add(const char* category, const char* name, float* value) {
	size_t idx = twk_add(category, name, GameSettingsItem::ST_FLOAT);
	_settingsCtx->items[idx].ptr.fPtr = value;
}

// -------------------------------------------------------
// add vec2
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec2* value) {
	size_t idx = twk_add(category, name, GameSettingsItem::ST_VEC2);
	_settingsCtx->items[idx].ptr.v2Ptr = value;
}

// -------------------------------------------------------
// add vec3
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec3* value) {
	size_t idx = twk_add(category, name, GameSettingsItem::ST_VEC3);
	_settingsCtx->items[idx].ptr.v3Ptr = value;
}

// -------------------------------------------------------
// add vec4
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec4* value) {
	size_t idx = twk_add(category, name, GameSettingsItem::ST_VEC4);
	_settingsCtx->items[idx].ptr.v4Ptr = value;
}

// -------------------------------------------------------
// add array
// -------------------------------------------------------
void twk_add(const char* category, const char* name, float* array, int size) {
	size_t idx = twk_add(category, name, GameSettingsItem::ST_ARRAY);
	_settingsCtx->items[idx].ptr.arPtr = array;
	_settingsCtx->items[idx].arrayLength = size;
}

// -------------------------------------------------------
// internal token struct
// -------------------------------------------------------
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
// internal char buffer reallocate
// -------------------------------------------------------
static void twk_realloc_char_buffer(int additional) {
	if (_settingsCtx->charBuffer.data == 0) {
		_settingsCtx->charBuffer.data = new char[additional];
		_settingsCtx->charBuffer.capacity = additional;
		_settingsCtx->charBuffer.indices = new int[16];
		_settingsCtx->charBuffer.sizes = new int[16];
	}
	else {
		char* tmp = new char[_settingsCtx->charBuffer.capacity + additional];
		memcpy(tmp, _settingsCtx->charBuffer.data, _settingsCtx->charBuffer.capacity);
		_settingsCtx->charBuffer.capacity = _settingsCtx->charBuffer.capacity + additional;
		delete[] _settingsCtx->charBuffer.data;
		_settingsCtx->charBuffer.data = tmp;
	}
}

// -------------------------------------------------------
// internal add string to char buffer
// -------------------------------------------------------
static int twk_add_string(const char* txt) {
	int l = strlen(txt);
	if ((l + _settingsCtx->charBuffer.size) >= _settingsCtx->charBuffer.capacity) {
		twk_realloc_char_buffer(l * 2);
	}
	int current = _settingsCtx->charBuffer.size;
	char* dest = _settingsCtx->charBuffer.data + current;
	_settingsCtx->charBuffer.indices[_settingsCtx->charBuffer.count] = current;
	_settingsCtx->charBuffer.sizes[_settingsCtx->charBuffer.count] = l;
	++_settingsCtx->charBuffer.count;
	strncpy(dest, txt, l);
	_settingsCtx->charBuffer.size += l;
	dest[l] = '\0';
	return _settingsCtx->charBuffer.count - 1;
}

// -------------------------------------------------------
// save
// -------------------------------------------------------
void twk_save() {
	char name[128];
	FILE* fp = fopen("test.txt", "w");
	int currentCategory = -1;
	if (fp) {
		for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
			const GameSettingsItem& item = _settingsCtx->items[i];
			if (item.categoryNameIndex != currentCategory) {
				if (currentCategory != -1) {
					fprintf(fp,"}\n");
				}
				int cidx = item.categoryNameIndex;
				char* src = _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[cidx];
				int l = _settingsCtx->charBuffer.sizes[cidx];
				strncpy(name, src, l);
				name[l] = '\0';
				fprintf(fp, "%s {\n", name);
				currentCategory = item.categoryNameIndex;
			}
			int nidx = item.nameIndex;
			char* src = _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[nidx];
			int l = _settingsCtx->charBuffer.sizes[nidx];
			strncpy(name, src, l);
			name[l] = '\0';
			fprintf(fp, "\t%s : ", name);
			switch (item.type) {
				case GameSettingsItem::ST_INT: fprintf(fp, "%d\n", *item.ptr.iPtr); break;
				case GameSettingsItem::ST_UINT: fprintf(fp, "%d\n", *item.ptr.uiPtr); break;
				case GameSettingsItem::ST_FLOAT: fprintf(fp, "%g\n", *item.ptr.fPtr); break;
				case GameSettingsItem::ST_VEC2 : fprintf(fp, "%g, %g\n", item.ptr.v2Ptr->x, item.ptr.v2Ptr->y); break;
				case GameSettingsItem::ST_VEC3: fprintf(fp, "%g, %g, %g\n", item.ptr.v3Ptr->x, item.ptr.v3Ptr->y, item.ptr.v3Ptr->z); break;
				case GameSettingsItem::ST_VEC4: fprintf(fp, "%g, %g, %g, %g\n", item.ptr.v4Ptr->x, item.ptr.v4Ptr->y, item.ptr.v4Ptr->z, item.ptr.v4Ptr->w); break;
				case GameSettingsItem::ST_COLOR: fprintf(fp, "%d, %d, %d, %d\n", (item.ptr.cPtr->r * 255.0f), (item.ptr.cPtr->g * 255.0f), (item.ptr.cPtr->b * 255.0f), (item.ptr.cPtr->a * 255.0f)); break;
				case GameSettingsItem::ST_ARRAY: {
					int la = item.arrayLength;
					for (int j = 0; j < la; ++j) {
						if (j != 0) {
							fprintf(fp, ", ");
						}
						fprintf(fp, "%g", item.ptr.arPtr[j]); 
					}
					fprintf(fp, "\n");
					break;
				}
			}
		}
		fprintf(fp, "}\n");
		fclose(fp);
	}
}

// -------------------------------------------------------
// settings item
// -------------------------------------------------------
static void twk_get_filetime(const char* fileName, FILETIME* time) {
	WORD ret = -1;
	// no file sharing mode
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		// Retrieve the file times for the file.
		GetFileTime(hFile, NULL, NULL, time);
		CloseHandle(hFile);
	}
}

// -------------------------------------------------------
// internal load file
// -------------------------------------------------------
static char* twk_load_file(const char* fileName, FILETIME* time) {
	int size = 0;
	FILE *fp = fopen(fileName, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* buffer = new char[sz + 1];
		fread(buffer, 1, sz, fp);
		buffer[sz] = '\0';
		fclose(fp);
		twk_get_filetime(fileName, time);
		return buffer;
	}
	return 0;
}

// -------------------------------------------------------
// internal is digit
// -------------------------------------------------------
static bool twk_is_digit(const char c) {
	return ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.');
}

// -------------------------------------------------------
// internal is numeric
// -------------------------------------------------------
static bool twk_is_numeric(const char c) {
	return ((c >= '0' && c <= '9'));
}

// -------------------------------------------------------
// internal is whitespace
// -------------------------------------------------------
static bool twk_is_whitespace(const char c) {
	if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
		return true;
	}
	return false;
}

// -------------------------------------------------------
// internal string to float
// -------------------------------------------------------
static float twk_strtof(const char* p, char** endPtr) {
	while (twk_is_whitespace(*p)) {
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
	while (twk_is_numeric(*p)) {
		value *= 10.0f;
		value = value + (*p - '0');
		++p;
	}
	if (*p == '.') {
		++p;
		float dec = 1.0f;
		float frac = 0.0f;
		while (twk_is_numeric(*p)) {
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

// -------------------------------------------------------
// internal find variable
// -------------------------------------------------------
static size_t twk_find(uint32_t categoryHash, const char* name) {
	uint32_t hash = twk_fnv1a(name);
	for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
		if (_settingsCtx->items[i].hash == hash && _settingsCtx->items[i].categoryHash == categoryHash) {
			return i;
		}
	}
	return -1;
}

// -------------------------------------------------------
// internal set value
// -------------------------------------------------------
static void twk_set_value(uint32_t catHash, int categoryNameIndex, const char* name, int nameIndex, int length, float* values, int count) {
	size_t idx = twk_find(catHash, name);
	if (idx != -1) {
		GameSettingsItem& item = _settingsCtx->items[idx];
		item.nameIndex = nameIndex;
		item.length = length;
		item.categoryNameIndex = categoryNameIndex;
		if (item.type == GameSettingsItem::ST_INT && count == 1) {
			*item.ptr.iPtr = values[0];
		}
		else if (item.type == GameSettingsItem::ST_UINT && count == 1) {
			*item.ptr.uiPtr = values[0];
		}
		else if (item.type == GameSettingsItem::ST_FLOAT && count == 1) {
			*item.ptr.fPtr = values[0];
		}
		else if (item.type == GameSettingsItem::ST_VEC2 && count == 2) {
			item.ptr.v2Ptr->x = values[0];
			item.ptr.v2Ptr->y = values[1];
		}
		else if (item.type == GameSettingsItem::ST_VEC3 && count == 3) {
			item.ptr.v3Ptr->x = values[0];
			item.ptr.v3Ptr->y = values[1];
			item.ptr.v3Ptr->z = values[2];
		}
		else if (item.type == GameSettingsItem::ST_VEC4 && count == 4) {
			item.ptr.v4Ptr->x = values[0];
			item.ptr.v4Ptr->y = values[1];
			item.ptr.v4Ptr->z = values[2];
			item.ptr.v4Ptr->w = values[3];
		}
		else if (item.type == GameSettingsItem::ST_COLOR && count == 4) {
			item.ptr.cPtr->r = values[0] / 255.0f;
			item.ptr.cPtr->g = values[1] / 255.0f;
			item.ptr.cPtr->b = values[2] / 255.0f;
			item.ptr.cPtr->a = values[3] / 255.0f;
		}
		else if (item.type == GameSettingsItem::ST_ARRAY && count == item.arrayLength) {
			for (int i = 0; i < count; ++i) {
				item.ptr.arPtr[i] = values[i];
			}
		}
	}
}

// -------------------------------------------------------
// internal check if the file should be (re)loaded
// -------------------------------------------------------
static bool twk_requires_loading() {
	if (!_settingsCtx->loaded) {
		return true;
	}
	FILETIME now;
	twk_get_filetime(_settingsCtx->fileName, &now);
	if (CompareFileTime(&_settingsCtx->filetime, &now) == -1) {
		_settingsCtx->filetime = now;
		return true;
	}
	return false;
}

// -------------------------------------------------------
// load
// -------------------------------------------------------
bool twk_load() {
	if (twk_requires_loading()) {
		_settingsCtx->loaded = true;
		int cnt = 0;
		const char* _text = twk_load_file(_settingsCtx->fileName, &_settingsCtx->filetime);
		const char* p = _text;
		std::vector<Token> tokens;
		while (*p != 0) {
			Token token(Token::EMPTY);
			if (twk_is_digit(*p)) {
				char *out;
				token = Token(Token::NUMBER, twk_strtof(p, &out));
				p = out;
			}
			else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
				const char *identifier = p;
				while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
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
				case ':': token = Token(Token::ASSIGN); break;
				case ',': token = Token(Token::DELIMITER); break;
				}
				++p;
			}
			if (token.type != Token::EMPTY) {
				tokens.push_back(token);
			}
		}
		int idx = 0;
		Token& t = tokens[idx];
		char name[128];
		float values[128];
		uint32_t catHash = 0;
		int currentCategory = -1;
		int currentName = -1;
		while (idx < tokens.size()) {
			if (t.type == Token::NAME) {
				strncpy(name, _text + t.index, t.size);
				name[t.size] = '\0';
				int strIdx = twk_add_string(name);
				++idx;
				Token& n = tokens[idx];
				if (n.type == Token::OPEN_BRACES) {
					catHash = twk_fnv1a(name);
					currentCategory = strIdx;
				}
				else if (n.type == Token::ASSIGN) {
					++idx;
					Token& v = tokens[idx];
					int count = 0;
					while (v.type == Token::NUMBER || v.type == Token::DELIMITER) {
						if (v.type == Token::NUMBER) {
							if (count < 128) {
								values[count++] = v.value;
							}
						}
						++idx;
						if (idx >= tokens.size()) {
							break;
						}
						v = tokens[idx];
					}
					twk_set_value(catHash, currentCategory, name, strIdx, t.size, values, count);
				}
				else {
					++idx;
				}
			}
			else {
				++idx;
			}
			if (idx < tokens.size()) {
				t = tokens[idx];
			}
		}
		delete[] _text;
		for (int i = 0; i < _settingsCtx->charBuffer.count; ++i) {
			char* source = _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[i];
			int length = _settingsCtx->charBuffer.sizes[i];
			strncpy(name, source, length);
			name[length] = '\0';
			printf("%d : %d / %d = %s\n", i, _settingsCtx->charBuffer.indices[i], _settingsCtx->charBuffer.sizes[i], name);
			
		}
		return true;
	}
	return false;
}

#endif // GAMESETTINGS_IMPLEMENTATION