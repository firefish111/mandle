.section .text
.globl mandelbrot

mandelbrot: # u8 iterations, f32 real_component[16], f32 imag_component[16], u8 output_hues[16]
        movq   %rcx, %r8  # stash output hues' location, as rcx has to be used for the loop
        movzbq %dil, %rcx # loop count

        vpxorq    %zmm0, %zmm0, %zmm0  # z0 = Re(z)
        vmovdqa64 %zmm0, %zmm1         # z1 = Im(z)

        vmovdqa64 (%rsi), %zmm2        # z2 = Re(c)
        vmovdqa64 (%rdx), %zmm3        # z3 = Im(c)

        pxor %xmm15, %xmm15 # hue mask
        kxnorw %k4, %k4, %k4 # k4=0xffff. k4 = running whether valid or not. do EVERYTHING with k4
  .iter:
        # z = z^2 + c. in other words:
        # Re(z) = Re(z)^2 - Im(z)^2 + Re(c)
        # Im(z) = 2 * Re(z) * Im(z) + Im(c)
        vmovdqa64    %zmm2, %zmm6      # z6 = backup Re(c). do NOT mask, as only eight bit mask is used there

        vmulps       %zmm0, %zmm0, %zmm4 {%k4} # z4 = Re(z)^2
        vfnmadd231ps %zmm1, %zmm1, %zmm6 {%k4} # z6 = -(Im(z)^2) + Re(c)

        # get euclidean norm
        vmovdqa64    %zmm4, %zmm8              # z8 = backup Re(z)^2
        vfmadd231ps  %zmm1, %zmm1, %zmm8 {%k4} # z8 = Re(z)^2 + Im(z)^2, a.k.a. |z|^2

        # compare if any number is too high. this is if |z| >= 2, or more easily computed, that |z|^2 >= 4.
        # write to k1 if z8 !< 4.0, using an unordered comparison (so always true for NaN, as only way to get NaN is (Inf - Inf) in this case).
        # full instruction means Vector CoMPare Not Less Than Unordered Quiet (i.e. no exceptions if NaN) over Packed Singles.
        # this also broadcasts the threshold using weird esoteric notation
        vcmpnlt_uqps (threshold) {1to16}, %zmm8, %k1 {%k4}
        vpbroadcastb %ecx, %xmm15 {%k1} # put least significant byte of ecx in every cell of xmm15 that was infinite (i.e. write to cells that are infinite, when they became infinite)
        kandnw       %k4, %k1, %k4 # invert those infinite ones in %k4, as there is no point continuing computation for them

        vaddps       %zmm0, %zmm0, %zmm5 {%k4} # z5 = 2 * Re(z)
        vfmadd132ps  %zmm5, %zmm3, %zmm1 {%k4} # z1 = 2 * Re(z) * Im(z) + Im(c); this is next Im(z)

        vaddps       %zmm4, %zmm6, %zmm0 {%k4} # z0 = Re(z)^2 - Im(z)^2 + Re(c); this is next Re(z)

        loop .iter

        movdqa %xmm15, (%r8) # write colour data

        ret

.section .rodata
threshold:
  .float 4.0 # the maximum, 2, but squared
