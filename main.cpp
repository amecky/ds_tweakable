#include "ds_tweakable.h"
#include <diesel.h>

int main() {
	float firstFloat = 42.0f;
	ds::vec2 vec = ds::vec2(100, 200);
	GameSettings settings("settings.txt");
	settings.add("test.value", &firstFloat, 0.0f);
	settings.add("test.more", &vec);
	settings.load();
	printf("firstFloat: %g\n", firstFloat);
	printf("vec %g %g\n", vec.x, vec.y);
    return 0;
}

