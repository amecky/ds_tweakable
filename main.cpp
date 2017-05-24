#define GAMESETTINGS_IMPLEMENTATION
#include "ds_tweakable.h"

int main() {
	twk_init("settings.json");
	float f = 100.0f;
	twk_add("test", "value", &f);
	ds::vec2 v2(211, 222);
	twk_add("test", "more", &v2);
	ds::vec3 v3(101, 201, 301);
	twk_add("test", "vthree", &v3);
	ds::vec4 v4(8,9,10,11);
	twk_add("test", "vfour", &v4);
	float ar[10];
	twk_add("test", "array", ar, 10);
	while (1 < 2) {
		if (twk_load()) {
			twk_save();
			printf("value = %g\n", f);
			printf("vec2 %g %g\n", v2.x, v2.y);
			printf("vec3 %g %g %g\n", v3.x, v3.y, v3.z);
			printf("vec4 %g %g %g %g\n", v4.x, v4.y, v4.z, v4.w);
			for (int i = 0; i < 10; ++i) {
				printf("ar[%d] = %g\n", i, ar[i]);
			}
		}
		Sleep(200);
	}
	twk_shutdown();
    return 0;
}

