#include <libavcodec/avcodec.h>
int main() {
    avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    return 0;
}
