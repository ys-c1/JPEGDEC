# Baseline profile (measured, pristine HEAD ff585f2, this machine)

Method: compile-time ablation (replace JPEGIDCT with a no-op / skip JPEGPutMCU*),
wall-clock min of 7 decodes to RGB8888, 3840x2160, `-O2`.

| image   | build       | full ms | no-IDCT | no-upsample | IDCT ms | upsample ms | IDCT+ups % |
|---------|-------------|---------|---------|-------------|---------|-------------|------------|
| smooth  | default SSE | 6.08    | 5.32    | 4.28        | 0.76    | 1.80        | 42 %       |
| texture | default SSE | 15.10   | 10.07   | 12.99       | 5.03    | 2.11        | 47 %       |
| text    | default SSE | 12.46   | 11.12   | 10.70       | 1.34    | 1.76        | 25 %       |
| noise   | default SSE | 40.63   | 32.72   | 39.16       | 7.91    | 1.47        | 23 %       |
| smooth  | -DNO_SIMD   | 19.41   | 18.75   | 4.53        | 0.67    | 14.89       | 80 %       |
| texture | -DNO_SIMD   | 29.02   | 23.61   | 14.24       | 5.40    | 14.78       | 70 %       |
| text    | -DNO_SIMD   | 28.68   | 25.24   | 12.83       | 3.43    | 15.85       | 67 %       |
| noise   | -DNO_SIMD   | 70.58   | 47.30   | 54.69       | 23.28   | 15.89       | 56 %       |
| grain   | -DNO_SIMD   | 58.71   | 40.10   | 41.40       | 18.60   | 17.31       | 61 %       |

=> The stated premise "70% of the time in IDCT + upsampling" holds for the
   portable C build (-DNO_SIMD), which is the path used on the MCU targets
   this library ships for (the P4/S31 SIMD converters are chroma-only; the
   IDCT is the portable C code there).  It does NOT hold for the x86 SSE2
   build, where entropy decoding dominates and the chroma converter is
   already SIMD.

Hard ceiling = full / (time with both IDCT and upsampling removed):
  -DNO_SIMD: smooth 5.46x, texture 3.54x, text 3.06x, noise 2.26x, grain 2.38x
  default  : smooth 1.72x, texture 1.84x, text 1.36x, noise 1.32x

Noise/grain can only reach 1.8x if the IDCT+upsample block becomes >4.5x
faster, because ~40-55% of the time is Huffman/VLC decoding, which is not
in scope (and, on SSE2, cannot be reached at all: ceiling 1.32x).
