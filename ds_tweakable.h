#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
#include <diesel.h>

typedef void(*twkErrorHandler)(const char* errorMessage);

void twk_init(const char* fileName, twkErrorHandler = 0);

void twk_add(const char* category, const char* name, int* value);

void twk_add(const char* category, const char* name, uint32_t* value);

void twk_add(const char* category, const char* name, float* value);

void twk_add(const char* category, const char* name, ds::vec2* value);

void twk_add(const char* category, const char* name, ds::vec3* value);

void twk_add(const char* category, const char* name, ds::vec4* value);

void twk_add(const char* category, const char* name, ds::Color* value);

void twk_add(const char* category, const char* name, float* array,int size);

void twk_shutdown();

bool twk_load();

void twk_parse(const char* text);

bool twk_verify();

void twk_save();

//#define GAMESETTINGS_IMPLEMENTATION

#ifdef GAMESETTINGS_IMPLEMENTATION

#include <Windows.h>
#include <vector>


// -------------------------------------------------------
// game settings item
// -------------------------------------------------------

struct TWKCategory {
	uint32_t hash;
	uint16_t nameIndex;
};

struct GameSettingsItem {

	enum SettingsType { ST_FLOAT, ST_INT, ST_UINT, ST_VEC2,	ST_VEC3, ST_VEC4, ST_COLOR,	ST_ARRAY, ST_NONE };

	size_t categoryIndex;
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
	bool found;
};

// -------------------------------------------------------
// internal char buffer
// -------------------------------------------------------
struct InternalCharBuffer {
	char* data;
	size_t capacity;
	size_t size;
	int count;
	size_t* indices;
	size_t* sizes;
	uint32_t* hashes;
	size_t indexCapacity;
};

// -------------------------------------------------------
// internal settings context
// -------------------------------------------------------
struct SettingsContext {
	const char* fileName;
	FILETIME filetime;
	std::vector<GameSettingsItem> items;
	std::vector<TWKCategory> categories;
	bool loaded;
	InternalCharBuffer charBuffer;
	twkErrorHandler errorHandler;
};

static SettingsContext* _settingsCtx = 0;

// -------------------------------------------------------
// init
// -------------------------------------------------------
void twk_init(const char* fileName, twkErrorHandler errorHandler) {
	_settingsCtx = new SettingsContext;
	_settingsCtx->fileName = fileName;
	_settingsCtx->loaded = false;
	_settingsCtx->charBuffer.data = 0;
	_settingsCtx->charBuffer.capacity = 0;
	_settingsCtx->charBuffer.count = 0;
	_settingsCtx->charBuffer.indices = 0;
	_settingsCtx->charBuffer.sizes = 0;
	_settingsCtx->charBuffer.size = 0;
	_settingsCtx->charBuffer.indexCapacity = 0;
	_settingsCtx->charBuffer.hashes = 0;
	_settingsCtx->errorHandler = errorHandler;
}

// -------------------------------------------------------
// shutdown
// -------------------------------------------------------
void twk_shutdown() {
	if (_settingsCtx != 0) {
		if (_settingsCtx->charBuffer.data != 0) {
			delete[] _settingsCtx->charBuffer.data;
		}
		if (_settingsCtx->charBuffer.indices != 0) {
			delete[] _settingsCtx->charBuffer.indices;
		}
		if (_settingsCtx->charBuffer.sizes != 0) {
			delete[] _settingsCtx->charBuffer.sizes;
		}
		delete _settingsCtx;
	}
}

// -------------------------------------------------------
// Hashing
// -------------------------------------------------------
const uint32_t TWK__FNV_Prime = 0x01000193; 
const uint32_t TWK__FNV_Seed = 0x811C9DC5;

inline uint32_t twk_fnv1a(const char* text, uint32_t hash = TWK__FNV_Seed) {
	const unsigned char* ptr = (const unsigned char*)text;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * TWK__FNV_Prime;
	}
	return hash;
}

static int twk__find_category(const char* category) {
	uint32_t categoryHash = twk_fnv1a(category);
	for (size_t i = 0; i < _settingsCtx->categories.size(); ++i) {
		if (_settingsCtx->categories[i].hash == categoryHash) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

// -------------------------------------------------------
// internal char buffer reallocation
// -------------------------------------------------------
static void twk__realloc_char_buffer(InternalCharBuffer* buffer, size_t additional) {
	if (buffer->data == 0) {
		buffer->data = new char[additional];
		buffer->capacity = additional;
		buffer->indices = new size_t[16];
		buffer->sizes = new size_t[16];
		buffer->indexCapacity = 16;
	}
	else {
		char* tmp = new char[buffer->capacity + additional];
		memcpy(tmp, buffer->data, buffer->capacity);
		buffer->capacity = buffer->capacity + additional;
		delete[] buffer->data;
		buffer->data = tmp;
	}
}

// -------------------------------------------------------
// internal char buffer indices reallocation
// -------------------------------------------------------
static void twk__reallocate_indices(InternalCharBuffer* buffer, size_t additional) {
	size_t* tmpi = new size_t[buffer->indexCapacity + additional];
	memcpy(tmpi, buffer->indices, buffer->count);
	delete[] buffer->indices;
	buffer->indices = tmpi;
	size_t* tmps = new size_t[buffer->indexCapacity + additional];
	memcpy(tmps, buffer->sizes, buffer->count);
	delete[] buffer->sizes;
	buffer->sizes = tmps;
	uint32_t* tmph = new uint32_t[buffer->indexCapacity + additional];
	memcpy(tmph, buffer->hashes, buffer->count);
	delete[] buffer->hashes;
	buffer->hashes = tmph;
	buffer->indexCapacity += additional;
}

static int twk__find_string(uint32_t hash) {
	for (int i = 0; i < _settingsCtx->charBuffer.count; ++i) {
		if (_settingsCtx->charBuffer.hashes[i] == hash) {
			return i;
		}
	}
	return -1;
}
// -------------------------------------------------------
// internal add string to char buffer
// -------------------------------------------------------
static int twk__add_string(const char* txt) {
	uint32_t hash = twk_fnv1a(txt);
	int strIdx = twk__find_string(hash);
	if (strIdx != -1) {
		return strIdx;
	}
	size_t l = strlen(txt);
	if ((l + _settingsCtx->charBuffer.size + 1) >= _settingsCtx->charBuffer.capacity) {
		twk__realloc_char_buffer(&_settingsCtx->charBuffer, l * 2);
	}
	if (_settingsCtx->charBuffer.count + 1 >= _settingsCtx->charBuffer.indexCapacity) {
		twk__reallocate_indices(&_settingsCtx->charBuffer, 16);
	}
	size_t current = _settingsCtx->charBuffer.size;
	char* dest = _settingsCtx->charBuffer.data + current;
	_settingsCtx->charBuffer.indices[_settingsCtx->charBuffer.count] = current;
	_settingsCtx->charBuffer.sizes[_settingsCtx->charBuffer.count] = l + 1;
	_settingsCtx->charBuffer.hashes[_settingsCtx->charBuffer.count] = hash;
	++_settingsCtx->charBuffer.count;
	strncpy(dest, txt, l);
	_settingsCtx->charBuffer.size += l + 1;
	dest[l] = '\0';
	return _settingsCtx->charBuffer.count - 1;
}

static char* twk__get_string(int index) {
	return _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[index];
}
// -------------------------------------------------------
// internal add
// -------------------------------------------------------
static size_t twk_internal_add(const char* category, const char* name, GameSettingsItem::SettingsType type) {
	GameSettingsItem item;
	int catIdx = twk__find_category(category);
	if (catIdx == -1) {
		TWKCategory cat;
		cat.hash = twk_fnv1a(category);
		cat.nameIndex = twk__add_string(category);
		_settingsCtx->categories.push_back(cat);
		catIdx = static_cast<int>(_settingsCtx->categories.size()) - 1;
	}
	item.categoryIndex = catIdx;
	item.hash = twk_fnv1a(name);
	item.type = type;
	item.arrayLength = 0;
	item.found = false;
	item.nameIndex = twk__add_string(name);
	_settingsCtx->items.push_back(item);
	return _settingsCtx->items.size() - 1;
}

// -------------------------------------------------------
// add int
// -------------------------------------------------------
void twk_add(const char* category, const char* name, int* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_INT);
	_settingsCtx->items[idx].ptr.iPtr = value;
}

// -------------------------------------------------------
// add uint32_t
// -------------------------------------------------------
void twk_add(const char* category, const char* name, uint32_t* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_UINT);
	_settingsCtx->items[idx].ptr.uiPtr = value;
}

// -------------------------------------------------------
// add float
// -------------------------------------------------------
void twk_add(const char* category, const char* name, float* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_FLOAT);
	_settingsCtx->items[idx].ptr.fPtr = value;
}

// -------------------------------------------------------
// add vec2
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec2* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_VEC2);
	_settingsCtx->items[idx].ptr.v2Ptr = value;
}

// -------------------------------------------------------
// add vec3
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec3* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_VEC3);
	_settingsCtx->items[idx].ptr.v3Ptr = value;
}

// -------------------------------------------------------
// add vec4
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::vec4* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_VEC4);
	_settingsCtx->items[idx].ptr.v4Ptr = value;
}

// -------------------------------------------------------
// add color
// -------------------------------------------------------
void twk_add(const char* category, const char* name, ds::Color* value) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_COLOR);
	_settingsCtx->items[idx].ptr.cPtr = value;
}

// -------------------------------------------------------
// add array
// -------------------------------------------------------
void twk_add(const char* category, const char* name, float* array, int size) {
	size_t idx = twk_internal_add(category, name, GameSettingsItem::ST_ARRAY);
	_settingsCtx->items[idx].ptr.arPtr = array;
	_settingsCtx->items[idx].arrayLength = size;
}

// ------------------------------------------------------------
// internal error reporting using the twkErrorHandle callback
// ------------------------------------------------------------
static void twk__report_error(char* format, ...) {
	if (_settingsCtx->errorHandler != 0) {
		va_list args;
		va_start(args, format);
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		int written = vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);	
		(*_settingsCtx->errorHandler)(buffer);
		va_end(args);
	}
}
// -------------------------------------------------------
// internal token struct
// -------------------------------------------------------
struct TWKToken {

	enum TokenType { EMPTY, NUMBER, NAME, DELIMITER, OPEN_BRACES, CLOSE_BRACES, ASSIGN };

	TWKToken() {}
	TWKToken(TokenType type) : type(type) {}
	TWKToken(TokenType type, float v) : type(type), value(v) {}
	TWKToken(TokenType type, int i, int s) : type(type), index(i), size(s) {}

	TokenType type;
	float value;
	int index;
	int size;
};



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
			if (item.categoryIndex != currentCategory) {
				if (currentCategory != -1) {
					fprintf(fp,"}\n");
				}
				int cidx = static_cast<int>(item.categoryIndex);
				int cnIdx = _settingsCtx->categories[cidx].nameIndex;
				char* src = _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[cnIdx];
				size_t l = _settingsCtx->charBuffer.sizes[cnIdx];
				strncpy(name, src, l);
				name[l] = '\0';
				fprintf(fp, "%s {\n", name);
				currentCategory = static_cast<int>(item.categoryIndex);
			}
			int nidx = item.nameIndex;
			char* src = _settingsCtx->charBuffer.data + _settingsCtx->charBuffer.indices[nidx];
			size_t l = _settingsCtx->charBuffer.sizes[nidx];
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
				case GameSettingsItem::ST_COLOR: {
					for (int j = 0; j < 4; ++j) {
						if (j != 0) {
							fprintf(fp, ", ");
						}
						int v = static_cast<int>(item.ptr.cPtr->values[j] * 255.0f);
						fprintf(fp, "%d", v);
					}
					fprintf(fp, "\n");
					break;
				}
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
static bool twk__get_filetime(const char* fileName, FILETIME* time) {
	WORD ret = -1;
	// no file sharing mode
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		// Retrieve the file times for the file.
		GetFileTime(hFile, NULL, NULL, time);
		CloseHandle(hFile);
		return true;
	}
	return false;
}

// -------------------------------------------------------
// internal load file
// -------------------------------------------------------
static char* twk__load_file(const char* fileName, FILETIME* time) {
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
		twk__get_filetime(fileName, time);
		return buffer;
	}
	else {
		twk__report_error("Cannot load file: '%s'", fileName);
	}
	return 0;
}
/*
 * The idea is taken from https://github.com/chadaustin/sajson/blob/master/include/sajson.h
 * bit 1 = digit
 * bit 2 = numeric
 * bit 3 = name
 * bit 4 = whitespace
 * bit 5 = supported
*/
const uint8_t PARSE_FLAGS[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 0, 0, 8, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	8, 0, 0,16, 0, 0, 0, 0, 0, 0, 0, 1,16, 1, 1, 0, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,16, 0, 0, 0, 0, 0, 
	0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0,16, 4,
	0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,16, 0,16, 0, 0,
	// 128-255 -> we do not support any character >= 128
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static bool inline twk__is_supported(const char c) {
	return (PARSE_FLAGS[static_cast<unsigned char>(c)]) > 0;
}
// -------------------------------------------------------
// internal is digit
// -------------------------------------------------------
static bool  inline twk__is_digit(const char c) {
	return (PARSE_FLAGS[static_cast<unsigned char>(c)] & 1) != 0;
}

// -------------------------------------------------------
// internal is numeric
// -------------------------------------------------------
static bool  inline twk__is_numeric(const char c) {
	return (PARSE_FLAGS[static_cast<unsigned char>(c)] & 2) != 0;
}

// -------------------------------------------------------
// internal is name char
// -------------------------------------------------------
static bool  inline twk__is_name(const char c) {
	return (PARSE_FLAGS[static_cast<unsigned char>(c)] & 4) != 0;
}
// -------------------------------------------------------
// internal is whitespace
// -------------------------------------------------------
static bool  inline twk__is_whitespace(const char c) {
	return (PARSE_FLAGS[static_cast<unsigned char>(c)] & 8) != 0;
}

// -------------------------------------------------------
// internal string to float
// -------------------------------------------------------
static float twk__strtof(const char* p, char** endPtr) {
	while (twk__is_whitespace(*p)) {
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
	while (twk__is_numeric(*p)) {
		value *= 10.0f;
		value = value + (*p - '0');
		++p;
	}
	if (*p == '.') {
		++p;
		float dec = 1.0f;
		float frac = 0.0f;
		while (twk__is_numeric(*p)) {
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
static size_t twk__find(int categoryIndex, const char* name) {
	uint32_t hash = twk_fnv1a(name);
	for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
		if (_settingsCtx->items[i].hash == hash && _settingsCtx->items[i].categoryIndex == categoryIndex) {
			return i;
		}
	}
	return -1;
}

// -------------------------------------------------------
// internal set value
// -------------------------------------------------------
static void twk__set_value(int categoryIndex, const char* name, int nameIndex, int length, float* values, int count) {
	size_t idx = twk__find(categoryIndex, name);
	if (idx != -1) {
		GameSettingsItem& item = _settingsCtx->items[idx];
		item.nameIndex = nameIndex;
		item.length = length;
		if (item.type == GameSettingsItem::ST_INT && count == 1) {
			*item.ptr.iPtr = static_cast<int>(values[0]);
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_UINT && count == 1) {
			*item.ptr.uiPtr = static_cast<uint32_t>(values[0]);
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_FLOAT && count == 1) {
			*item.ptr.fPtr = values[0];
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_VEC2 && count == 2) {
			item.ptr.v2Ptr->x = values[0];
			item.ptr.v2Ptr->y = values[1];
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_VEC3 && count == 3) {
			item.ptr.v3Ptr->x = values[0];
			item.ptr.v3Ptr->y = values[1];
			item.ptr.v3Ptr->z = values[2];
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_VEC4 && count == 4) {
			item.ptr.v4Ptr->x = values[0];
			item.ptr.v4Ptr->y = values[1];
			item.ptr.v4Ptr->z = values[2];
			item.ptr.v4Ptr->w = values[3];
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_COLOR && count == 4) {
			for (int i = 0; i < 4; ++i) {
				item.ptr.cPtr->values[i] = values[i] / 255.0f;
			}
			item.found = true;
		}
		else if (item.type == GameSettingsItem::ST_ARRAY && count == item.arrayLength) {
			for (int i = 0; i < count; ++i) {
				item.ptr.arPtr[i] = values[i];
			}
			item.found = true;
		}
	}
}

// -------------------------------------------------------
// internal check if the file should be (re)loaded
// -------------------------------------------------------
static bool twk__requires_loading() {
	if (!_settingsCtx->loaded) {
		return true;
	}
	FILETIME now;
	if (twk__get_filetime(_settingsCtx->fileName, &now)) {
		if (CompareFileTime(&_settingsCtx->filetime, &now) == -1) {
			_settingsCtx->filetime = now;
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------
// internal check if the file should be (re)loaded
// -------------------------------------------------------
void twk_parse(const char* text) {
	// FIXME: reset internal char buffer - keep the already allocated memory
	// do not clear added variables
	//_settingsCtx->charBuffer.count = 0;
	//_settingsCtx->charBuffer.size = 0;
	// reset found to false for every item
	for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
		_settingsCtx->items[i].found = false;
	}
	const char* p = text;
	std::vector<TWKToken> tokens;
	while (*p != 0) {
		TWKToken token(TWKToken::EMPTY);
		if (twk__is_supported(*p)) {
			if (twk__is_digit(*p)) {
				char *out;
				token = TWKToken(TWKToken::NUMBER, twk__strtof(p, &out));
				p = out;
			}
			else if (twk__is_name(*p)) {
				const char *identifier = p;
				while (twk__is_name(*p)) {
					p++;
				}
				token = TWKToken(TWKToken::NAME, identifier - text, p - identifier);
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
				case '{': token = TWKToken(TWKToken::OPEN_BRACES); break;
				case '}': token = TWKToken(TWKToken::CLOSE_BRACES); break;
				case ' ': case '\t': case '\n': case '\r': break;
				case ':': token = TWKToken(TWKToken::ASSIGN); break;
				case ',': token = TWKToken(TWKToken::DELIMITER); break;
				}
				++p;
			}
			if (token.type != TWKToken::EMPTY) {
				tokens.push_back(token);
			}
		}
		else {
			++p;
		}
	}
	int idx = 0;
	TWKToken& t = tokens[idx];
	char name[128];
	float values[128];
	int currentCategory = -1;
	int currentName = -1;
	while (idx < tokens.size()) {
		if (t.type == TWKToken::NAME) {
			strncpy(name, text + t.index, t.size);
			name[t.size] = '\0';
			++idx;
			TWKToken& n = tokens[idx];
			if (n.type == TWKToken::OPEN_BRACES) {
				int cidx = twk__find_category(name);
				if (cidx == -1) {
					TWKCategory cat;
					cat.hash = twk_fnv1a(name);
					cat.nameIndex = twk__add_string(name);
					_settingsCtx->categories.push_back(cat);
					currentCategory = static_cast<int>(_settingsCtx->categories.size()) - 1;
				}		
				else {
					currentCategory = cidx;
				}
			}
			else if (n.type == TWKToken::ASSIGN) {
				int strIdx = twk__add_string(name);
				++idx;
				TWKToken& v = tokens[idx];
				int count = 0;
				while (v.type == TWKToken::NUMBER || v.type == TWKToken::DELIMITER) {
					if (v.type == TWKToken::NUMBER) {
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
				twk__set_value(currentCategory, name, strIdx, t.size, values, count);
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
	for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
		const GameSettingsItem& item = _settingsCtx->items[i];
		if (!item.found) {
			char* in = twk__get_string(item.nameIndex);
			twk__report_error("Item '%s' not found", in);
		}
	}
}

// -------------------------------------------------------
// load
// -------------------------------------------------------
bool twk_load() {
	if (twk__requires_loading()) {
		_settingsCtx->loaded = true;
		int cnt = 0;
		const char* _text = twk__load_file(_settingsCtx->fileName, &_settingsCtx->filetime);
		if (_text != 0) {
			twk_parse(_text);
			delete[] _text;
			return true;
		}
		return false;
	}
	return false;
}

bool twk_verify() {
	for (size_t i = 0; i < _settingsCtx->items.size(); ++i) {
		const GameSettingsItem& item = _settingsCtx->items[i];
		if (!item.found) {
			return false;
		}
	}
	return true;
}

#endif // GAMESETTINGS_IMPLEMENTATION