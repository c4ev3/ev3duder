#define packet_alloc(type, extra) _packet_alloc(sizeof(type), extra, &type##_INIT)
void *_packet_alloc(size_t size, size_t extra, void *init)
{
	void *ptr = malloc(size + extra);
	memcpy(ptr, init, size);
	((SYSTEM_CMD *)ptr)->packetLen = size + extra - PREFIX_SIZE;
	return ptr;
}
