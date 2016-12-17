#ifdef ESP32
#define LOGV(...) ESP_LOGV(tag, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(tag, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(tag, __VA_ARGS__)
#else
#define LOGV(...)  { \
	printf("(V) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#define LOGE(...)  { \
	printf("(E) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#define LOGD(...)  { \
	printf("(D) %s: ", tag); \
	printf(__VA_ARGS__); \
	printf("\n"); \
}
#endif
