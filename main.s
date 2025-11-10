	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"main.c"
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	__real@3f800000                 # -- Begin function main
	.section	.rdata,"dr",discard,__real@3f800000
	.p2align	2, 0x0
__real@3f800000:
	.long	0x3f800000                      # float 1
	.globl	__xmm@00000000000000003f19999a3f4ccccd
	.section	.rdata,"dr",discard,__xmm@00000000000000003f19999a3f4ccccd
	.p2align	4, 0x0
__xmm@00000000000000003f19999a3f4ccccd:
	.long	0x3f4ccccd                      # float 0.800000011
	.long	0x3f19999a                      # float 0.600000024
	.zero	4
	.zero	4
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
	.globl	__real@41200000
	.section	.rdata,"dr",discard,__real@41200000
	.p2align	2, 0x0
__real@41200000:
	.long	0x41200000                      # float 10
	.globl	__real@42f00000
	.section	.rdata,"dr",discard,__real@42f00000
	.p2align	2, 0x0
__real@42f00000:
	.long	0x42f00000                      # float 120
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
	.globl	__real@41a00000
	.section	.rdata,"dr",discard,__real@41a00000
	.p2align	2, 0x0
__real@41a00000:
	.long	0x41a00000                      # float 20
	.globl	__real@44fa0000
	.section	.rdata,"dr",discard,__real@44fa0000
	.p2align	2, 0x0
__real@44fa0000:
	.long	0x44fa0000                      # float 2000
	.text
	.globl	main
	.p2align	4
main:                                   # @main
.seh_proc main
# %bb.0:
	pushq	%r15
	.seh_pushreg %r15
	pushq	%r14
	.seh_pushreg %r14
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rdi
	.seh_pushreg %rdi
	pushq	%rbx
	.seh_pushreg %rbx
	subq	$624, %rsp                      # imm = 0x270
	.seh_stackalloc 624
	movaps	%xmm6, 608(%rsp)                # 16-byte Spill
	.seh_savexmm %xmm6, 608
	.seh_endprologue
	callq	*__imp_NU_Init(%rip)
	movl	$-1, %esi
	testl	%eax, %eax
	je	.LBB0_7
# %bb.1:
	leaq	"??_C@_07DIOLDIJH@app?4xml?$AA@"(%rip), %rcx
	callq	*__imp_NU_From_XML(%rip)
	testl	%eax, %eax
	je	.LBB0_7
# %bb.2:
	leaq	"??_C@_07GEBBGION@app?4css?$AA@"(%rip), %rdx
	leaq	120(%rsp), %rsi
	movq	%rsi, %rcx
	callq	*__imp_NU_Stylesheet_Create(%rip)
	movq	%rsi, %rcx
	callq	*__imp_NU_Stylesheet_Apply(%rip)
	movss	__real@3f800000(%rip), %xmm0    # xmm0 = [1.0E+0,0.0E+0,0.0E+0,0.0E+0]
	movlps	%xmm0, 104(%rsp)
	movl	$0, 112(%rsp)
	movsd	__xmm@00000000000000003f19999a3f4ccccd(%rip), %xmm0 # xmm0 = [8.00000011E-1,6.00000024E-1,0.0E+0,0.0E+0]
	movsd	%xmm0, 88(%rsp)
	movl	$1058642330, 96(%rsp)           # imm = 0x3F19999A
	leaq	"??_C@_05PGKDIPIO@chart?$AA@"(%rip), %rcx
	callq	*__imp_NU_Get_Node_By_Id(%rip)
	movl	%eax, %esi
	leaq	88(%rsp), %r14
	movq	%r14, 56(%rsp)
	leaq	104(%rsp), %rdi
	movq	%rdi, 48(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1128792064, 32(%rsp)           # imm = 0x43480000
	movq	__imp_NU_Border_Rect(%rip), %rbx
	movss	__real@42c80000(%rip), %xmm1    # xmm1 = [1.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@437a0000(%rip), %xmm2    # xmm2 = [2.5E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@41200000(%rip), %xmm6    # xmm6 = [1.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movl	%eax, %ecx
	movaps	%xmm6, %xmm3
	callq	*%rbx
	movq	%r14, 56(%rsp)
	movq	%rdi, 48(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1128792064, 32(%rsp)           # imm = 0x43480000
	movss	__real@42f00000(%rip), %xmm1    # xmm1 = [1.2E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43480000(%rip), %xmm2    # xmm2 = [2.0E+2,0.0E+0,0.0E+0,0.0E+0]
	movl	%esi, %ecx
	movaps	%xmm6, %xmm3
	callq	*%rbx
	movq	%r14, 56(%rsp)
	movq	%rdi, 48(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1128792064, 32(%rsp)           # imm = 0x43480000
	movss	__real@430c0000(%rip), %xmm1    # xmm1 = [1.4E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43660000(%rip), %xmm2    # xmm2 = [2.3E+2,0.0E+0,0.0E+0,0.0E+0]
	movl	%esi, %ecx
	movaps	%xmm6, %xmm3
	callq	*%rbx
	movq	%r14, 56(%rsp)
	movq	%rdi, 48(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1128792064, 32(%rsp)           # imm = 0x43480000
	movss	__real@43200000(%rip), %xmm1    # xmm1 = [1.6E+2,0.0E+0,0.0E+0,0.0E+0]
	movss	__real@43700000(%rip), %xmm2    # xmm2 = [2.4E+2,0.0E+0,0.0E+0,0.0E+0]
	movl	%esi, %ecx
	movaps	%xmm6, %xmm3
	callq	*%rbx
	movq	%rdi, 48(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1133903872, 32(%rsp)           # imm = 0x43960000
	movss	__real@41a00000(%rip), %xmm6    # xmm6 = [2.0E+1,0.0E+0,0.0E+0,0.0E+0]
	movl	%esi, %ecx
	movaps	%xmm6, %xmm1
	movaps	%xmm6, %xmm2
	movaps	%xmm6, %xmm3
	callq	*__imp_NU_Line(%rip)
	movl	$83952901, 84(%rsp)             # imm = 0x5010505
	movq	%rdi, 64(%rsp)
	leaq	84(%rsp), %rax
	movq	%rax, 48(%rsp)
	movl	$4, 56(%rsp)
	movl	$1065353216, 40(%rsp)           # imm = 0x3F800000
	movl	$1101004800, 32(%rsp)           # imm = 0x41A00000
	movss	__real@44fa0000(%rip), %xmm3    # xmm3 = [2.0E+3,0.0E+0,0.0E+0,0.0E+0]
	movl	%esi, %ecx
	movaps	%xmm6, %xmm1
	movaps	%xmm6, %xmm2
	callq	*__imp_NU_Dashed_Line(%rip)
	movl	$100, %edi
	movq	__imp_NU_Create_Node(%rip), %rbx
	movq	__imp_NU_NODE(%rip), %r14
	leaq	"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"(%rip), %r15
	.p2align	4
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	movl	%esi, %ecx
	movl	$1, %edx
	callq	*%rbx
	movl	%eax, %ecx
	callq	*%r14
	movq	%r15, 24(%rax)
	decl	%edi
	jne	.LBB0_3
# %bb.4:
	movq	__imp_NU_Running(%rip), %rsi
	.p2align	4
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	callq	*%rsi
	testl	%eax, %eax
	jne	.LBB0_5
# %bb.6:
	callq	*__imp_NU_Quit(%rip)
	leaq	120(%rsp), %rcx
	callq	*__imp_NU_Stylesheet_Free(%rip)
	xorl	%esi, %esi
.LBB0_7:
	movl	%esi, %eax
	movaps	608(%rsp), %xmm6                # 16-byte Reload
	addq	$624, %rsp                      # imm = 0x270
	popq	%rbx
	popq	%rdi
	popq	%rsi
	popq	%r14
	popq	%r15
	retq
	.seh_endproc
                                        # -- End function
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

	.section	.rdata,"dr",discard,"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"
	.globl	"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@" # @"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@"
"??_C@_0BA@MJDHNFAI@the?5string?5char?$AA@":
	.asciz	"the string char"

	.addrsig
	.globl	_fltused
