	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"main.c"
	.def	sprintf;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,sprintf
	.globl	sprintf                         # -- Begin function sprintf
	.p2align	4
sprintf:                                # @sprintf
.seh_proc sprintf
# %bb.0:
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	movq	%r9, 104(%rsp)
	movq	%r8, 96(%rsp)
	movq	%rdx, 64(%rsp)
	movq	%rcx, 56(%rsp)
	leaq	96(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	40(%rsp), %r9
	movq	64(%rsp), %rdx
	movq	56(%rsp), %rcx
	xorl	%eax, %eax
	movl	%eax, %r8d
	callq	_vsprintf_l
	movl	%eax, 52(%rsp)
	movl	52(%rsp), %eax
	addq	$72, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	vsprintf;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,vsprintf
	.globl	vsprintf                        # -- Begin function vsprintf
	.p2align	4
vsprintf:                               # @vsprintf
.seh_proc vsprintf
# %bb.0:
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	movq	%r8, 64(%rsp)
	movq	%rdx, 56(%rsp)
	movq	%rcx, 48(%rsp)
	movq	64(%rsp), %rax
	movq	56(%rsp), %r8
	movq	48(%rsp), %rcx
	movq	$-1, %rdx
	xorl	%r9d, %r9d
                                        # kill: def $r9 killed $r9d
	movq	%rax, 32(%rsp)
	callq	_vsnprintf_l
	nop
	addq	$72, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	_snprintf;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,_snprintf
	.globl	_snprintf                       # -- Begin function _snprintf
	.p2align	4
_snprintf:                              # @_snprintf
.seh_proc _snprintf
# %bb.0:
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	movq	%r9, 104(%rsp)
	movq	%r8, 64(%rsp)
	movq	%rdx, 56(%rsp)
	movq	%rcx, 48(%rsp)
	leaq	104(%rsp), %rax
	movq	%rax, 32(%rsp)
	movq	32(%rsp), %r9
	movq	64(%rsp), %r8
	movq	56(%rsp), %rdx
	movq	48(%rsp), %rcx
	callq	_vsnprintf
	movl	%eax, 44(%rsp)
	movl	44(%rsp), %eax
	addq	$72, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	_vsnprintf;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,_vsnprintf
	.globl	_vsnprintf                      # -- Begin function _vsnprintf
	.p2align	4
_vsnprintf:                             # @_vsnprintf
.seh_proc _vsnprintf
# %bb.0:
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	movq	%r9, 64(%rsp)
	movq	%r8, 56(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rcx, 40(%rsp)
	movq	64(%rsp), %rax
	movq	56(%rsp), %r8
	movq	48(%rsp), %rdx
	movq	40(%rsp), %rcx
	xorl	%r9d, %r9d
                                        # kill: def $r9 killed $r9d
	movq	%rax, 32(%rsp)
	callq	_vsnprintf_l
	nop
	addq	$72, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	_vsprintf_l;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,_vsprintf_l
	.globl	_vsprintf_l                     # -- Begin function _vsprintf_l
	.p2align	4
_vsprintf_l:                            # @_vsprintf_l
.seh_proc _vsprintf_l
# %bb.0:
	subq	$72, %rsp
	.seh_stackalloc 72
	.seh_endprologue
	movq	%r9, 64(%rsp)
	movq	%r8, 56(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rcx, 40(%rsp)
	movq	64(%rsp), %rax
	movq	56(%rsp), %r9
	movq	48(%rsp), %r8
	movq	40(%rsp), %rcx
	movq	$-1, %rdx
	movq	%rax, 32(%rsp)
	callq	_vsnprintf_l
	nop
	addq	$72, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	_vsnprintf_l;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,_vsnprintf_l
	.globl	_vsnprintf_l                    # -- Begin function _vsnprintf_l
	.p2align	4
_vsnprintf_l:                           # @_vsnprintf_l
.seh_proc _vsnprintf_l
# %bb.0:
	subq	$136, %rsp
	.seh_stackalloc 136
	.seh_endprologue
	movq	176(%rsp), %rax
	movq	%r9, 128(%rsp)
	movq	%r8, 120(%rsp)
	movq	%rdx, 112(%rsp)
	movq	%rcx, 104(%rsp)
	movq	176(%rsp), %rax
	movq	%rax, 88(%rsp)                  # 8-byte Spill
	movq	128(%rsp), %rax
	movq	%rax, 80(%rsp)                  # 8-byte Spill
	movq	120(%rsp), %rax
	movq	%rax, 72(%rsp)                  # 8-byte Spill
	movq	112(%rsp), %rax
	movq	%rax, 64(%rsp)                  # 8-byte Spill
	movq	104(%rsp), %rax
	movq	%rax, 56(%rsp)                  # 8-byte Spill
	callq	__local_stdio_printf_options
	movq	56(%rsp), %rdx                  # 8-byte Reload
	movq	64(%rsp), %r8                   # 8-byte Reload
	movq	72(%rsp), %r9                   # 8-byte Reload
	movq	80(%rsp), %r10                  # 8-byte Reload
	movq	%rax, %rcx
	movq	88(%rsp), %rax                  # 8-byte Reload
	movq	(%rcx), %rcx
	orq	$1, %rcx
	movq	%r10, 32(%rsp)
	movq	%rax, 40(%rsp)
	callq	__stdio_common_vsprintf
	movl	%eax, 100(%rsp)
	cmpl	$0, 100(%rsp)
	jge	.LBB5_2
# %bb.1:
	movl	$4294967295, %eax               # imm = 0xFFFFFFFF
	movl	%eax, 52(%rsp)                  # 4-byte Spill
	jmp	.LBB5_3
.LBB5_2:
	movl	100(%rsp), %eax
	movl	%eax, 52(%rsp)                  # 4-byte Spill
.LBB5_3:
	movl	52(%rsp), %eax                  # 4-byte Reload
	addq	$136, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	__local_stdio_printf_options;
	.scl	2;
	.type	32;
	.endef
	.section	.text,"xr",discard,__local_stdio_printf_options
	.globl	__local_stdio_printf_options    # -- Begin function __local_stdio_printf_options
	.p2align	4
__local_stdio_printf_options:           # @__local_stdio_printf_options
# %bb.0:
	leaq	__local_stdio_printf_options._OptionsStorage(%rip), %rax
	retq
                                        # -- End function
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	__real@41a00000                 # -- Begin function main
	.section	.rdata,"dr",discard,__real@41a00000
	.p2align	2, 0x0
__real@41a00000:
	.long	0x41a00000                      # float 20
	.globl	__real@44fa0000
	.section	.rdata,"dr",discard,__real@44fa0000
	.p2align	2, 0x0
__real@44fa0000:
	.long	0x44fa0000                      # float 2000
	.globl	__real@3f800000
	.section	.rdata,"dr",discard,__real@3f800000
	.p2align	2, 0x0
__real@3f800000:
	.long	0x3f800000                      # float 1
	.globl	__real@43960000
	.section	.rdata,"dr",discard,__real@43960000
	.p2align	2, 0x0
__real@43960000:
	.long	0x43960000                      # float 300
	.globl	__real@43200000
	.section	.rdata,"dr",discard,__real@43200000
	.p2align	2, 0x0
__real@43200000:
	.long	0x43200000                      # float 160
	.globl	__real@43700000
	.section	.rdata,"dr",discard,__real@43700000
	.p2align	2, 0x0
__real@43700000:
	.long	0x43700000                      # float 240
	.globl	__real@41200000
	.section	.rdata,"dr",discard,__real@41200000
	.p2align	2, 0x0
__real@41200000:
	.long	0x41200000                      # float 10
	.globl	__real@43480000
	.section	.rdata,"dr",discard,__real@43480000
	.p2align	2, 0x0
__real@43480000:
	.long	0x43480000                      # float 200
	.globl	__real@430c0000
	.section	.rdata,"dr",discard,__real@430c0000
	.p2align	2, 0x0
__real@430c0000:
	.long	0x430c0000                      # float 140
	.globl	__real@43660000
	.section	.rdata,"dr",discard,__real@43660000
	.p2align	2, 0x0
__real@43660000:
	.long	0x43660000                      # float 230
	.globl	__real@42f00000
	.section	.rdata,"dr",discard,__real@42f00000
	.p2align	2, 0x0
__real@42f00000:
	.long	0x42f00000                      # float 120
	.globl	__real@42c80000
	.section	.rdata,"dr",discard,__real@42c80000
	.p2align	2, 0x0
__real@42c80000:
	.long	0x42c80000                      # float 100
	.globl	__real@437a0000
	.section	.rdata,"dr",discard,__real@437a0000
	.p2align	2, 0x0
__real@437a0000:
	.long	0x437a0000                      # float 250
	.globl	__real@3f19999a
	.section	.rdata,"dr",discard,__real@3f19999a
	.p2align	2, 0x0
__real@3f19999a:
	.long	0x3f19999a                      # float 0.600000024
	.globl	__real@3f4ccccd
	.section	.rdata,"dr",discard,__real@3f4ccccd
	.p2align	2, 0x0
__real@3f4ccccd:
	.long	0x3f4ccccd                      # float 0.800000011
	.text
	.globl	main
	.p2align	4
main:                                   # @main
.seh_proc main
# %bb.0:
	subq	$632, %rsp                      # imm = 0x278
	.seh_stackalloc 632
	.seh_endprologue
	movl	$0, 628(%rsp)
	callq	*__imp_NU_Init(%rip)
	cmpl	$0, %eax
	jne	.LBB7_2
# %bb.1:
	movl	$-1, 628(%rsp)
	jmp	.LBB7_12
.LBB7_2:
	leaq	"??_C@_07DIOLDIJH@app?4xml?$AA@"(%rip), %rcx
	callq	*__imp_NU_From_XML(%rip)
	cmpl	$0, %eax
	jne	.LBB7_4
# %bb.3:
	movl	$-1, 628(%rsp)
	jmp	.LBB7_12
.LBB7_4:
	leaq	136(%rsp), %rcx
	leaq	"??_C@_07GEBBGION@app?4css?$AA@"(%rip), %rdx
	callq	*__imp_NU_Stylesheet_Create(%rip)
	leaq	136(%rsp), %rcx
	callq	*__imp_NU_Stylesheet_Apply(%rip)
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	movss	%xmm0, 124(%rsp)
	xorps	%xmm0, %xmm0
	movss	%xmm0, 128(%rsp)
	xorps	%xmm0, %xmm0
	movss	%xmm0, 132(%rsp)
	movss	__real@3f4ccccd(%rip), %xmm0    # xmm0 = [8.00000011E-1,0.0E+0,0.0E+0,0.0E+0]
	movss	%xmm0, 112(%rsp)
	movss	__real@3f19999a(%rip), %xmm0    # xmm0 = [6.00000024E-1,0.0E+0,0.0E+0,0.0E+0]
	movss	%xmm0, 116(%rsp)
	movss	__real@3f19999a(%rip), %xmm0    # xmm0 = [6.00000024E-1,0.0E+0,0.0E+0,0.0E+0]
	movss	%xmm0, 120(%rsp)
	leaq	"??_C@_05PGKDIPIO@chart?$AA@"(%rip), %rcx
	callq	*__imp_NU_Get_Node_By_Id(%rip)
	movl	%eax, 108(%rsp)
	movl	108(%rsp), %ecx
	movss	__real@42c80000(%rip), %xmm1    # xmm1 = [1.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@437a0000(%rip), %xmm2    # xmm2 = [2.5E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@41200000(%rip), %xmm3    # xmm3 = [1.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43480000(%rip), %xmm4    # xmm4 = [2.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rdx
	leaq	112(%rsp), %rax
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rax, 56(%rsp)
	callq	*__imp_NU_Border_Rect(%rip)
	movl	108(%rsp), %ecx
	movss	__real@42f00000(%rip), %xmm1    # xmm1 = [1.2E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43480000(%rip), %xmm4    # xmm4 = [2.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@41200000(%rip), %xmm3    # xmm3 = [1.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rdx
	leaq	112(%rsp), %rax
	movaps	%xmm4, %xmm2
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rax, 56(%rsp)
	callq	*__imp_NU_Border_Rect(%rip)
	movl	108(%rsp), %ecx
	movss	__real@430c0000(%rip), %xmm1    # xmm1 = [1.4E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43660000(%rip), %xmm2    # xmm2 = [2.3E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@41200000(%rip), %xmm3    # xmm3 = [1.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43480000(%rip), %xmm4    # xmm4 = [2.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rdx
	leaq	112(%rsp), %rax
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rax, 56(%rsp)
	callq	*__imp_NU_Border_Rect(%rip)
	movl	108(%rsp), %ecx
	movss	__real@43200000(%rip), %xmm1    # xmm1 = [1.6E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43700000(%rip), %xmm2    # xmm2 = [2.4E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@41200000(%rip), %xmm3    # xmm3 = [1.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43480000(%rip), %xmm4    # xmm4 = [2.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rdx
	leaq	112(%rsp), %rax
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rdx, 48(%rsp)
	movq	%rax, 56(%rsp)
	callq	*__imp_NU_Border_Rect(%rip)
	movl	108(%rsp), %ecx
	movss	__real@41a00000(%rip), %xmm3    # xmm3 = [2.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43960000(%rip), %xmm4    # xmm4 = [3.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rax
	movaps	%xmm3, %xmm1
	movaps	%xmm3, %xmm2
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rax, 48(%rsp)
	callq	*__imp_NU_Line(%rip)
	movl	.L__const.main.dash_pattern(%rip), %eax
	movl	%eax, 104(%rsp)
	leaq	104(%rsp), %rdx
	movl	108(%rsp), %ecx
	movss	__real@41a00000(%rip), %xmm4    # xmm4 = [2.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@44fa0000(%rip), %xmm3    # xmm3 = [2.0E+3,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	leaq	124(%rsp), %rax
	movaps	%xmm4, %xmm1
	movaps	%xmm4, %xmm2
	movss	%xmm4, 32(%rsp)
	movss	%xmm0, 40(%rsp)
	movq	%rdx, 48(%rsp)
	movl	$4, 56(%rsp)
	movq	%rax, 64(%rsp)
	callq	*__imp_NU_Dashed_Line(%rip)
	leaq	"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"(%rip), %rax
	movq	%rax, 96(%rsp)
	movl	$0, 92(%rsp)
.LBB7_5:                                # =>This Inner Loop Header: Depth=1
	cmpl	$100, 92(%rsp)
	jge	.LBB7_8
# %bb.6:                                #   in Loop: Header=BB7_5 Depth=1
	movl	108(%rsp), %ecx
	movl	$1, %edx
	callq	*__imp_NU_Create_Node(%rip)
	movl	%eax, 88(%rsp)
	movq	96(%rsp), %rax
	movq	%rax, 80(%rsp)                  # 8-byte Spill
	movl	88(%rsp), %ecx
	callq	*__imp_NU_NODE(%rip)
	movq	80(%rsp), %rcx                  # 8-byte Reload
	movq	%rcx, 24(%rax)
# %bb.7:                                #   in Loop: Header=BB7_5 Depth=1
	movl	92(%rsp), %eax
	addl	$1, %eax
	movl	%eax, 92(%rsp)
	jmp	.LBB7_5
.LBB7_8:
	jmp	.LBB7_9
.LBB7_9:                                # =>This Inner Loop Header: Depth=1
	callq	*__imp_NU_Running(%rip)
	cmpl	$0, %eax
	je	.LBB7_11
# %bb.10:                               #   in Loop: Header=BB7_9 Depth=1
	jmp	.LBB7_9
.LBB7_11:
	callq	*__imp_NU_Quit(%rip)
	leaq	136(%rsp), %rcx
	callq	*__imp_NU_Stylesheet_Free(%rip)
.LBB7_12:
	movl	628(%rsp), %eax
	addq	$632, %rsp                      # imm = 0x278
	retq
	.seh_endproc
                                        # -- End function
	.lcomm	__local_stdio_printf_options._OptionsStorage,8,8 # @__local_stdio_printf_options._OptionsStorage
	.section	.rdata,"dr",discard,"??_C@_07DIOLDIJH@app?4xml?$AA@"
	.globl	"??_C@_07DIOLDIJH@app?4xml?$AA@" # @"??_C@_07DIOLDIJH@app?4xml?$AA@"
"??_C@_07DIOLDIJH@app?4xml?$AA@":
	.asciz	"app.xml"

	.section	.rdata,"dr",discard,"??_C@_07GEBBGION@app?4css?$AA@"
	.globl	"??_C@_07GEBBGION@app?4css?$AA@" # @"??_C@_07GEBBGION@app?4css?$AA@"
"??_C@_07GEBBGION@app?4css?$AA@":
	.asciz	"app.css"

	.section	.rdata,"dr",discard,"??_C@_05PGKDIPIO@chart?$AA@"
	.globl	"??_C@_05PGKDIPIO@chart?$AA@"   # @"??_C@_05PGKDIPIO@chart?$AA@"
"??_C@_05PGKDIPIO@chart?$AA@":
	.asciz	"chart"

	.section	.rdata,"dr"
.L__const.main.dash_pattern:            # @__const.main.dash_pattern
	.ascii	"\005\005\001\005"

	.section	.rdata,"dr",discard,"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"
	.globl	"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@" # @"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"
"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@":
	.asciz	"the string char"

	.addrsig
	.addrsig_sym _vsnprintf
	.addrsig_sym _vsprintf_l
	.addrsig_sym _vsnprintf_l
	.addrsig_sym __stdio_common_vsprintf
	.addrsig_sym __local_stdio_printf_options
	.addrsig_sym __local_stdio_printf_options._OptionsStorage
	.globl	_fltused
