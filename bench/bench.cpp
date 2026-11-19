// Benchmark / verification driver for the local JPEGDEC fork.
// Decodes a JPEG to RGB8888 into a caller-supplied framebuffer and emits
// either raw pixels (for PSNR) or timing information.
//
// usage: bench <file.jpg> <mode> [iters]
//   mode = time     -> prints median decode ms for [iters] runs
//   mode = dump     -> writes <file>.rgba raw output (w*h*4)
//   mode = info     -> prints width/height
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "JPEGDEC.h"

static unsigned char *filebuf = NULL;
static uint8_t *fb = NULL;

static int draw(JPEGDRAW *pDraw)
{
    (void)pDraw;
    return 1;
}

static int load(const char *path, unsigned char **buf, int *size)
{
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    *buf = (unsigned char *)malloc(n);
    if (fread(*buf, 1, n, f) != (size_t)n) { fclose(f); return 0; }
    fclose(f); *size = (int)n;
    return 1;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int g_cb_calls = 0;
static int draw_count(JPEGDRAW *pDraw) { (void)pDraw; g_cb_calls++; return 1; }

int main(int argc, char **argv)
{
    int size, w, h, mode_time, iters = 10;
    JPEGDEC jpg;
    if (argc < 3) { fprintf(stderr, "usage: %s file.jpg time|dump|info [iters]\n", argv[0]); return 2; }
    mode_time = !strcmp(argv[2], "time");
    if (argc > 3) iters = atoi(argv[3]);
    if (!load(argv[1], &filebuf, &size)) { fprintf(stderr, "cannot read %s\n", argv[1]); return 2; }
    if (!jpg.openRAM(filebuf, size, draw)) { fprintf(stderr, "open failed\n"); return 2; }
    w = jpg.getWidth(); h = jpg.getHeight();
    if (!strcmp(argv[2], "info")) { printf("%d %d\n", w, h); return 0; }
    fb = (uint8_t *)malloc((size_t)w * h * 4 + (size_t)(w + 64) * 64 * 4 + 4096);
    if (!strcmp(argv[2], "dump")) {
        jpg.setPixelType(RGB8888);
        jpg.setFramebuffer(fb);
        int ok = jpg.decode(0, 0, 0);
        if (!ok) { fprintf(stderr, "decode failed %d\n", jpg.getLastError()); return 2; }
        char out[1024]; snprintf(out, sizeof(out), "%s.rgba", argv[1]);
        FILE *f = fopen(out, "wb");
        fwrite(fb, 1, (size_t)w * h * 4, f);
        fclose(f);
        jpg.close();
        return 0;
    }
    // timing mode: decode() consumes parser state, so reopen for every run
    double best = 1e9, total = 0;
    for (int i = 0; i < iters + 1; i++) {
        if (!jpg.openRAM(filebuf, size, draw)) { fprintf(stderr, "open failed\n"); return 2; }
        jpg.setPixelType(RGB8888);
        jpg.setFramebuffer(fb);
        double t0 = now_ms();
        if (!jpg.decode(0, 0, 0)) { fprintf(stderr, "decode failed\n"); return 2; }
        double dt = now_ms() - t0;
        jpg.close();
        if (i == 0) continue; // warmup
        total += dt;
        if (dt < best) best = dt;
    }
    printf("{\"file\":\"%s\",\"w\":%d,\"h\":%d,\"itz\":%d,\"min_ms\":%.3f,\"avg_ms\":%.3f}\n",
           argv[1], w, h, iters, best, total / iters);
    jpg.close();
    return 0;
}
