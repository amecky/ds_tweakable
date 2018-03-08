# ds_tweakable

Tweaking game settings - to be used in games based on diesel

## Include

```
#define GAMESETTINGS_IMPLEMENTATION
#include <ds_tweakable.h>
```

## Initialize

```
twk_init("content\\settings.json");
```

## Adding tweakables

There are a number of methods available to add tweakables:

```
void twk_add(const char* category, const char* name, int* value);

void twk_add(const char* category, const char* name, uint32_t* value);

void twk_add(const char* category, const char* name, float* value);

void twk_add(const char* category, const char* name, ds::vec2* value);

void twk_add(const char* category, const char* name, ds::vec3* value);

void twk_add(const char* category, const char* name, ds::vec4* value);

void twk_add(const char* category, const char* name, ds::Color* value);

void twk_add(const char* category, const char* name, float* array,int size);
```

### Example

```
struct SparkleSettings {
    float gap;
    float ttl;
    float startScale;
    float endScale;
    float velocity;
    float velocityVariance;
}
```

```
sparkle { 
    gap : 2
    ttl : 0.6 
    start_scale : 1
    end_scale : 0.4 
    velocity : 80 
    velocity_variance : 60
}

```
Here is an example using the json above:
```
SparkleSettings settings;

twk_add("sparkle","gap", &settings.gap);
twk_add("sparkle","ttl", &settings.ttl);
twk_add("sparkle","start_scale", &settings.startScale);
```


## Loading the file

In order to load the file call:
```
twk_load();
```
In order to use the hot reload functionality you need to call this method periodically.

## Shutdown

You need to call twk_shutdown to clean up the used memory.
```
twk_shutdown();
```

## Loading text file as resource

### Add text file as resource

First we need to add the text file as resource. Add the following to resource.h:

```
#define IDS_SETTINGS					103
```

Next step is to add the actual in the resource.rc:
```
IDS_SETTINGS            RCDATA                  "content\\settings.json"  
```

Here is a method to load the text file from the resources:
```
const char* loadTextFile(const char* name) {
	
	HRSRC resourceHandle = ::FindResource(NULL, MAKEINTRESOURCE(IDS_SETTINGS), RT_RCDATA);
	if (resourceHandle == 0) {
		return 0;
	}
	DWORD imageSize = ::SizeofResource(NULL, resourceHandle);
	if (imageSize == 0) {
		return 0;
	}
	HGLOBAL myResourceData = ::LoadResource(NULL, resourceHandle);
	char* pMyBinaryData = (char*)::LockResource(myResourceData);
	UnlockResource(myResourceData);
	char* ret = new char[imageSize];
	memcpy(ret, pMyBinaryData, imageSize);
	FreeResource(myResourceData);
	return ret;
}
```
Since we are loading the content from a resource we need to call a different twk_init method:
```
#ifdef DEBUG
	twk_init("content\\settings.json");
#else
	twk_init();
#endif

	// use twk_add and whatver

#ifdef DEBUG
	twk_load();
#else
	const char* txt = loadTextFile("content\\settings.json");
	if (txt != 0) {
		twk_parse(txt);
		delete[] txt;
	}
#endif
```
There is an internal *reloadable* flag which is set to false if you use the twk_init() method.
Therefore it is actually safe to call twk_load.

### Using ds_tweakable and ds_imgui

This example demonstrates a GUI implementation:

```
void show_tweakable_gui(const char* category) {
	Tweakable  vars[256];
	int num = twk_get_tweakables(category, vars, 256);
	for (int i = 0; i < num; ++i) {
		if (vars[i].type == TweakableType::ST_FLOAT) {
			gui::Input(vars[i].name, vars[i].ptr.fPtr);
		}
		else if (vars[i].type == TweakableType::ST_INT) {
			gui::Input(vars[i].name, vars[i].ptr.iPtr);
		}
		else if (vars[i].type == TweakableType::ST_VEC2) {
			gui::Input(vars[i].name, vars[i].ptr.v2Ptr);
		}
		else if (vars[i].type == TweakableType::ST_VEC3) {
			gui::Input(vars[i].name, vars[i].ptr.v3Ptr);
		}
		else if (vars[i].type == TweakableType::ST_VEC4) {
			gui::Input(vars[i].name, vars[i].ptr.v4Ptr);
		}
		else if (vars[i].type == TweakableType::ST_COLOR) {
			gui::Input(vars[i].name, vars[i].ptr.cPtr);
		}
	}
}
```