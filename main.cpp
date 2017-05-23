#include "ds_tweakable.h"
#include <diesel.h>

int main() {
	float firstFloat = 42.0f;
	ds::vec2 vec = ds::vec2(100, 200);
	GameSettings settings("settings.txt");
	ds::Color clr;
	ds::vec3 v3;
	ds::vec4 v4;
	int v;
	uint32_t uiv;
	settings.add("test.value", &firstFloat, 0.0f);
	settings.add("test.more", &vec);
	settings.add("color", &clr);
	settings.add("vthree", &v3);
	settings.add("vfour", &v4);
	settings.add("iv", &v);
	settings.add("uiv",&uiv);

	settings.load();
	printf("firstFloat: %g\n", firstFloat);
	printf("vec2 %g %g\n", vec.x, vec.y);
	printf("vec3 %g %g %g\n", v3.x, v3.y, v3.z);
	printf("vec4 %g %g %g %g\n", v4.x, v4.y, v4.z, v4.w);
	printf("color %g %g %g %g\n", clr.r, clr.g,clr.b,clr.a);
	printf("iv: %d\n", v);
	printf("uv: %d\n", uiv);
	settings.save("test.txt");
    return 0;
}

