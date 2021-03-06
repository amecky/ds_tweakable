#define GAMESETTINGS_IMPLEMENTATION
#include "..\ds_tweakable.h"
#include "PerfTimer.h"

struct CatTest {
	float value;
	ds::Color clr;
	ds::vec2 vec;
};

void errorHandler(const char* errorMessage) {
	printf("%s\n", errorMessage);
}

void timingTest() {
	PerfTimer timer;
	timer.start();
	twk_init("..\\test\\settings.json", &errorHandler);
	twk_load();
	double elapsed = timer.stop();
	printf("elapsed: %3.6f microseconds\n", elapsed);
	twk_shutdown();
}

void verifyTest() {
	twk_init("..\\test\\settings.json", &errorHandler);
	float t = 100.0f;
	twk_add("border_settings", "Tension", &t);
	twk_load();
	if (!twk_verify()) {
		printf("ERROR - not valid\n");
	}
	twk_shutdown();
}

void categoryTest() {
	twk_init("..\\test\\categories.json", &errorHandler);
	CatTest cats[5];
	const char* NAMES[5] = { "cat_one","cat_two","cat_three","cat_four","cat_five" };
	for (int i = 0; i < 5; ++i) {
		twk_add(NAMES[i], "value", &cats[i].value);
		twk_add(NAMES[i], "clr", &cats[i].clr);
		twk_add(NAMES[i], "vec", &cats[i].vec);
	}
	twk_load();
	if (!twk_verify()) {
		printf("ERROR - not valid\n");
	}
	int num = twk_num_categories();
	printf("categories: %d\n", num);
	for (int i = 0; i < num; ++i) {
		printf("%d = %s\n", i, twk_get_category_name(i));
	}
	Tweakable items[32];
	int nr = twk_get_tweakables(0,items,32);
	for (int i = 0; i < nr; ++i) {
		printf("%d = %s (%d)\n", i, items[i].name,items[i].type);
	}
	printf("------------\n");
	for (int i = 0; i < 5; ++i) {
		printf("value %d: %g\n", i, cats[i].value);
		printf("vec %d: %g %g\n", i, cats[i].vec.x, cats[i].vec.y);
	}
	twk_shutdown();
}

void basicTest() {
	PerfTimer timer;
	twk_init("..\\test\\basic_settings.json", &errorHandler);
	float f = 100.0f;
	twk_add("test", "value", &f);
	ds::vec2 v2(211, 222);
	twk_add("test", "more", &v2);
	ds::vec3 v3(101, 201, 301);
	twk_add("test", "vthree", &v3);
	ds::vec4 v4(8, 9, 10, 11);
	twk_add("test", "vfour", &v4);
	float ar[10] = { 1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0,9.0f,10.0f };
	twk_add("test", "array", ar, 10);
	float none = 200.0f;
	twk_add("test", "none", &none);
	timer.start();
	while (1 < 2) {
		if (twk_load()) {
			double elapsed = timer.stop();
			printf("elapsed: %3.6f microseconds\n", elapsed);

			twk_save();
			printf("value = %g\n", f);
			printf("vec2 %g %g\n", v2.x, v2.y);
			printf("vec3 %g %g %g\n", v3.x, v3.y, v3.z);
			printf("vec4 %g %g %g %g\n", v4.x, v4.y, v4.z, v4.w);
			for (int i = 0; i < 10; ++i) {
				printf("ar[%d] = %g\n", i, ar[i]);
			}

			float tt = 0.0f;
			if (twk_get("next", "next_one", &tt)) {
				printf("tt: %g\n", tt);
			}
			else {
				printf("NO TT FOUND!!!\n");
			}
		}
		Sleep(200);
	}
	twk_shutdown();
}



int main() {
	
	//timingTest();

	//verifyTest();
	
	//basicTest();

	categoryTest();

    return 0;
}

