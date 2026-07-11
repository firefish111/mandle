.section .text
.globl mandelbrot

mandelbrot: # u8 iterations (dil), u8 * output_hues (rsi), f32x16 real_component (z0), f32x16 imag_component (z1)
        movzbq %dil, %rcx # loop count

        # z0 = Re(z); z1 = Im(z). technically starts off at 0, but we can optimise by precalculating the first round by setting z to c
        # z2 = Re(c); z3 = Im(c)
        # Re and Im of c are passed in z0 and z1 respectively. this allows us to easily do our z = c optimisation, by doing absolutely nothing
        # instead, we must copy Re and Im of c to z2 and z3, where we expect c to be.

        vmovdqa64 %zmm0, %zmm2  # z2 = Re(c)
        vmovdqa64 %zmm1, %zmm3  # z3 = Im(c)

        pxor %xmm15, %xmm15 # hue mask
        kxnorw %k4, %k4, %k4 # k4=0xffff. k4 = running whether valid or not. do EVERYTHING with k4

        vpbroadcastd (threshold), %zmm12 # comparison. is much slower to read from memory, so we pre-broadcast
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
        # this means that NaN, which can only possibly mean reached infinity far too quickly in this case, is treated as such
        # full instruction means Vector CoMPare Not Less Than Unordered Quiet (i.e. no exceptions if NaN) over Packed Singles.
        vcmpnlt_uqps %zmm12, %zmm8, %k1 {%k4}
        vpbroadcastb %ecx, %xmm15 {%k1} # put least significant byte of ecx in every cell of xmm15 that was infinite (i.e. write to cells that are infinite, when they became infinite)
        kandnw       %k4, %k1, %k4 # invert those infinite ones in %k4, as there is no point continuing computation for them

        vaddps       %zmm0, %zmm0, %zmm5 {%k4} # z5 = 2 * Re(z)
        vfmadd132ps  %zmm5, %zmm3, %zmm1 {%k4} # z1 = 2 * Re(z) * Im(z) + Im(c); this is next Im(z)

        vaddps       %zmm4, %zmm6, %zmm0 {%k4} # z0 = Re(z)^2 - Im(z)^2 + Re(c); this is next Re(z)

        loop .iter

        movdqa %xmm15, (%rsi) # write colour data

        ret

.section .rodata
threshold:
  .float 4.0 # the maximum, 2, but squared
