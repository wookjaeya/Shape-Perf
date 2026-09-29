## _mlir_ciface_main_graph_model
    28b0:	push   %rbp
    28b1:	push   %r15
    28b3:	push   %r14
    28b5:	push   %r13
    28b7:	push   %r12
    28b9:	push   %rbx
    28ba:	push   %rax
    28bb:	mov    %rdi,%rbx
    28be:	mov    0x8(%rsi),%r14
    28c2:	mov    $0x60010,%edi
    28c7:	call   21d0 <malloc@plt>
    28cc:	mov    %rax,(%rsp)
    28d0:	add    $0xf,%rax
    28d4:	and    $0xfffffffffffffff0,%rax
    28d8:	lea    0x1c(%rax),%rdx
    28dc:	xor    %esi,%esi
    28de:	vmovaps 0x371a(%rip),%ymm0        # 6000 <_fini+0x68c>
    28e6:	vmovaps 0x3732(%rip),%ymm1        # 6020 <_fini+0x6ac>
    28ee:	vmovaps 0x374a(%rip),%ymm2        # 6040 <_fini+0x6cc>
    28f6:	vmovaps 0x3762(%rip),%ymm3        # 6060 <_fini+0x6ec>
    28fe:	vmovaps 0x377a(%rip),%ymm4        # 6080 <_fini+0x70c>
    2906:	vmovaps 0x3792(%rip),%ymm5        # 60a0 <_fini+0x72c>
    290e:	vmovaps 0x37aa(%rip),%ymm6        # 60c0 <_fini+0x74c>
    2916:	vmovaps 0x37c2(%rip),%ymm7        # 60e0 <_fini+0x76c>
    291e:	vmovaps 0x3b58(%rip),%zmm8        # 6480 <_fini+0xb0c>
    2928:	vmovaps 0x3b8e(%rip),%zmm9        # 64c0 <_fini+0xb4c>
    2932:	mov    $0xcc,%dil
    2935:	vmovaps 0x3bc1(%rip),%zmm10        # 6500 <_fini+0xb8c>
    293f:	vmovaps 0x3bf7(%rip),%zmm11        # 6540 <_fini+0xbcc>
    2949:	vmovaps 0x3c2d(%rip),%zmm12        # 6580 <_fini+0xc0c>
    2953:	vmovaps 0x3c63(%rip),%zmm13        # 65c0 <_fini+0xc4c>
    295d:	vmovaps 0x3c99(%rip),%zmm14        # 6600 <_fini+0xc8c>
    2967:	vmovaps 0x3ccf(%rip),%zmm15        # 6640 <_fini+0xccc>
    2971:	vmovaps 0x3785(%rip),%ymm16        # 6100 <_fini+0x78c>
    297b:	vmovaps 0x379b(%rip),%ymm17        # 6120 <_fini+0x7ac>
    2985:	vmovaps 0x37b1(%rip),%ymm18        # 6140 <_fini+0x7cc>
    298f:	vmovaps 0x37c7(%rip),%ymm19        # 6160 <_fini+0x7ec>
    2999:	vmovaps 0x37dd(%rip),%ymm20        # 6180 <_fini+0x80c>
    29a3:	vmovaps 0x37f3(%rip),%ymm21        # 61a0 <_fini+0x82c>
    29ad:	vmovaps 0x3809(%rip),%ymm22        # 61c0 <_fini+0x84c>
    29b7:	vmovaps 0x381f(%rip),%ymm23        # 61e0 <_fini+0x86c>
    29c1:	mov    %r14,%r8
    29c4:	jmp    29eb <_mlir_ciface_main_graph_model+0x13b>
    29c6:	cs nopw 0x0(%rax,%rax,1)
    29d0:	inc    %rsi
    29d3:	add    $0x8000,%rdx
    29da:	add    $0x100,%r8
    29e1:	cmp    $0xc,%rsi
    29e5:	je     2d77 <_mlir_ciface_main_graph_model+0x4c7>
    29eb:	mov    %rsi,%rcx
    29ee:	shl    $0xf,%rcx
    29f2:	lea    (%rax,%rcx,1),%r9
    29f6:	lea    0x8000(%rax,%rcx,1),%rcx
    29fe:	mov    %rsi,%r10
    2a01:	shl    $0x8,%r10
    2a05:	lea    0x5f500(%r14,%r10,1),%r11
    2a0d:	cmp    %r11,%r9
    2a10:	setb   %bpl
    2a14:	add    %r14,%r10
    2a17:	cmp    %rcx,%r10
    2a1a:	setb   %r11b
    2a1e:	and    %bpl,%r11b
    2a21:	mov    %r8,%r15
    2a24:	mov    %rdx,%r12
    2a27:	xor    %r13d,%r13d
    2a2a:	jmp    2cb6 <_mlir_ciface_main_graph_model+0x406>
    2a2f:	nop
    2a30:	lea    (%r10,%r13,4),%rcx
    2a34:	vxorps %xmm24,%xmm24,%xmm24
    2a3a:	kxnorb %k0,%k0,%k1
    2a3e:	vgatherdps (%rcx,%ymm0,1),%ymm24{%k1}
    2a45:	vxorps %xmm25,%xmm25,%xmm25
    2a4b:	kxnorb %k0,%k0,%k1
    2a4f:	vgatherdps (%rcx,%ymm1,1),%ymm25{%k1}
    2a56:	vxorps %xmm26,%xmm26,%xmm26
    2a5c:	kxnorb %k0,%k0,%k1
    2a60:	vgatherdps (%rcx,%ymm2,1),%ymm26{%k1}
    2a67:	vxorps %xmm27,%xmm27,%xmm27
    2a6d:	kxnorb %k0,%k0,%k1
    2a71:	vgatherdps (%rcx,%ymm3,1),%ymm27{%k1}
    2a78:	vxorps %xmm28,%xmm28,%xmm28
    2a7e:	kxnorb %k0,%k0,%k1
    2a82:	vgatherdps (%rcx,%ymm4,1),%ymm28{%k1}
    2a89:	vxorps %xmm29,%xmm29,%xmm29
    2a8f:	kxnorb %k0,%k0,%k1
    2a93:	vgatherdps (%rcx,%ymm5,1),%ymm29{%k1}
    2a9a:	vxorps %xmm30,%xmm30,%xmm30
    2aa0:	kxnorb %k0,%k0,%k1
    2aa4:	vgatherdps (%rcx,%ymm6,1),%ymm30{%k1}
    2aab:	vxorps %xmm31,%xmm31,%xmm31
    2ab1:	kxnorb %k0,%k0,%k1
    2ab5:	vgatherdps (%rcx,%ymm7,1),%ymm31{%k1}
    2abc:	mov    %r13,%rbp
    2abf:	shl    $0x9,%rbp
    2ac3:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm24
    2aca:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm25
    2ad1:	vinsertf64x4 $0x1,%ymm29,%zmm28,%zmm26
    2ad8:	vinsertf64x4 $0x1,%ymm31,%zmm30,%zmm27
    2adf:	vmovaps %zmm26,%zmm28
    2ae5:	vpermt2ps %zmm27,%zmm8,%zmm28
    2aeb:	vmovaps %zmm24,%zmm29
    2af1:	vpermt2ps %zmm25,%zmm9,%zmm29
    2af7:	kmovd  %edi,%k1
    2afb:	vmovapd %zmm28,%zmm29{%k1}
    2b01:	vmovaps %zmm26,%zmm28
    2b07:	vpermt2ps %zmm27,%zmm10,%zmm28
    2b0d:	vmovaps %zmm26,%zmm30
    2b13:	vpermt2ps %zmm27,%zmm12,%zmm30
    2b19:	vmovaps %zmm24,%zmm31
    2b1f:	vpermt2ps %zmm25,%zmm13,%zmm31
    2b25:	vmovapd %zmm30,%zmm31{%k1}
    2b2b:	vmovaps %zmm24,%zmm30
    2b31:	vpermt2ps %zmm25,%zmm11,%zmm30
    2b37:	vpermt2ps %zmm27,%zmm14,%zmm26
    2b3d:	vpermt2ps %zmm25,%zmm15,%zmm24
    2b43:	vmovapd %zmm26,%zmm24{%k1}
    2b49:	vmovupd %zmm24,0xc0(%r9,%rbp,1)
    2b51:	vmovupd %zmm31,0x80(%r9,%rbp,1)
    2b59:	vmovapd %zmm28,%zmm30{%k1}
    2b5f:	vmovupd %zmm30,0x40(%r9,%rbp,1)
    2b67:	vmovupd %zmm29,(%r9,%rbp,1)
    2b6e:	vxorpd %xmm24,%xmm24,%xmm24
    2b74:	kxnorb %k0,%k0,%k2
    2b78:	vgatherdps (%rcx,%ymm16,1),%ymm24{%k2}
    2b7f:	vxorps %xmm25,%xmm25,%xmm25
    2b85:	kxnorb %k0,%k0,%k2
    2b89:	vgatherdps (%rcx,%ymm17,1),%ymm25{%k2}
    2b90:	vxorps %xmm26,%xmm26,%xmm26
    2b96:	kxnorb %k0,%k0,%k2
    2b9a:	vgatherdps (%rcx,%ymm18,1),%ymm26{%k2}
    2ba1:	vxorps %xmm27,%xmm27,%xmm27
    2ba7:	kxnorb %k0,%k0,%k2
    2bab:	vgatherdps (%rcx,%ymm19,1),%ymm27{%k2}
    2bb2:	vxorps %xmm28,%xmm28,%xmm28
    2bb8:	kxnorb %k0,%k0,%k2
    2bbc:	vgatherdps (%rcx,%ymm20,1),%ymm28{%k2}
    2bc3:	vxorpd %xmm29,%xmm29,%xmm29
    2bc9:	kxnorb %k0,%k0,%k2
    2bcd:	vgatherdps (%rcx,%ymm21,1),%ymm29{%k2}
    2bd4:	vxorpd %xmm30,%xmm30,%xmm30
    2bda:	kxnorb %k0,%k0,%k2
    2bde:	vgatherdps (%rcx,%ymm22,1),%ymm30{%k2}
    2be5:	vxorpd %xmm31,%xmm31,%xmm31
    2beb:	kxnorb %k0,%k0,%k2
    2bef:	vgatherdps (%rcx,%ymm23,1),%ymm31{%k2}
    2bf6:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm24
    2bfd:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm25
    2c04:	vinsertf64x4 $0x1,%ymm29,%zmm28,%zmm26
    2c0b:	vinsertf64x4 $0x1,%ymm31,%zmm30,%zmm27
    2c12:	vmovaps %zmm26,%zmm28
    2c18:	vpermt2ps %zmm27,%zmm8,%zmm28
    2c1e:	vmovaps %zmm24,%zmm29
    2c24:	vpermt2ps %zmm25,%zmm9,%zmm29
    2c2a:	vmovapd %zmm28,%zmm29{%k1}
    2c30:	vmovaps %zmm26,%zmm28
    2c36:	vpermt2ps %zmm27,%zmm10,%zmm28
    2c3c:	vmovaps %zmm24,%zmm30
    2c42:	vpermt2ps %zmm25,%zmm11,%zmm30
    2c48:	vmovapd %zmm28,%zmm30{%k1}
    2c4e:	vmovaps %zmm26,%zmm28
    2c54:	vpermt2ps %zmm27,%zmm12,%zmm28
    2c5a:	vmovaps %zmm24,%zmm31
    2c60:	vpermt2ps %zmm25,%zmm13,%zmm31
    2c66:	vmovapd %zmm28,%zmm31{%k1}
    2c6c:	vpermt2ps %zmm27,%zmm14,%zmm26
    2c72:	vpermt2ps %zmm25,%zmm15,%zmm24
    2c78:	vmovapd %zmm26,%zmm24{%k1}
    2c7e:	vmovupd %zmm24,0x1c0(%r9,%rbp,1)
    2c86:	vmovupd %zmm31,0x180(%r9,%rbp,1)
    2c8e:	vmovupd %zmm30,0x140(%r9,%rbp,1)
    2c96:	vmovupd %zmm29,0x100(%r9,%rbp,1)
    2c9e:	inc    %r13
    2ca1:	add    $0x200,%r12
    2ca8:	add    $0x4,%r15
    2cac:	cmp    $0x40,%r13
    2cb0:	je     29d0 <_mlir_ciface_main_graph_model+0x120>
    2cb6:	test   %r11b,%r11b
    2cb9:	je     2a30 <_mlir_ciface_main_graph_model+0x180>
    2cbf:	mov    $0xfffffffffffffff8,%rcx
    2cc6:	mov    %r15,%rbp
    2cc9:	nopl   0x0(%rax)
    2cd0:	vmovss 0x0(%rbp),%xmm24
    2cd7:	vmovss %xmm24,0x4(%r12,%rcx,4)
    2cdf:	vmovss 0xc00(%rbp),%xmm24
    2ce9:	vmovss %xmm24,0x8(%r12,%rcx,4)
    2cf1:	vmovss 0x1800(%rbp),%xmm24
    2cfb:	vmovss %xmm24,0xc(%r12,%rcx,4)
    2d03:	vmovss 0x2400(%rbp),%xmm24
    2d0d:	vmovss %xmm24,0x10(%r12,%rcx,4)
    2d15:	vmovss 0x3000(%rbp),%xmm24
    2d1f:	vmovss %xmm24,0x14(%r12,%rcx,4)
    2d27:	vmovss 0x3c00(%rbp),%xmm24
    2d31:	vmovss %xmm24,0x18(%r12,%rcx,4)
    2d39:	vmovss 0x4800(%rbp),%xmm24
    2d43:	vmovss %xmm24,0x1c(%r12,%rcx,4)
    2d4b:	vmovss 0x5400(%rbp),%xmm24
    2d55:	vmovss %xmm24,0x20(%r12,%rcx,4)
    2d5d:	add    $0x8,%rcx
    2d61:	add    $0x6000,%rbp
    2d68:	cmp    $0x78,%rcx
    2d6c:	jb     2cd0 <_mlir_ciface_main_graph_model+0x420>
    2d72:	jmp    2c9e <_mlir_ciface_main_graph_model+0x3ee>
    2d77:	mov    (%rsp),%rcx
    2d7b:	mov    %rcx,(%rbx)
    2d7e:	mov    %rax,0x8(%rbx)
    2d82:	vmovaps 0x34b6(%rip),%ymm0        # 6240 <_fini+0x8cc>
    2d8a:	vmovups %ymm0,0x10(%rbx)
    2d8f:	vmovaps 0x34c9(%rip),%ymm0        # 6260 <_fini+0x8ec>
    2d97:	vmovups %ymm0,0x30(%rbx)
    2d9c:	movq   $0x1,0x50(%rbx)
    2da4:	add    $0x8,%rsp
    2da8:	pop    %rbx
    2da9:	pop    %r12
    2dab:	pop    %r13
    2dad:	pop    %r14
    2daf:	pop    %r15
    2db1:	pop    %rbp
    2db2:	vzeroupper
    2db5:	ret
    2db6:	cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2150:	jmp    *0x6f3a(%rip)        # 9090 <_mlir_ciface_main_graph_model@@Base+0x67e0>
    2156:	push   $0x12
    215b:	jmp    2020 <_init+0x20>
## main_graph_model
    23a0:	push   %rbp
    23a1:	push   %r15
    23a3:	push   %r14
    23a5:	push   %r13
    23a7:	push   %r12
    23a9:	push   %rbx
    23aa:	push   %rax
    23ab:	mov    %rdx,%r14
    23ae:	mov    %rdi,%rbx
    23b1:	mov    $0x60010,%edi
    23b6:	call   21d0 <malloc@plt>
    23bb:	mov    %rax,(%rsp)
    23bf:	add    $0xf,%rax
    23c3:	and    $0xfffffffffffffff0,%rax
    23c7:	lea    0x1c(%rax),%rdx
    23cb:	xor    %esi,%esi
    23cd:	vmovaps 0x3c2b(%rip),%ymm0        # 6000 <_fini+0x68c>
    23d5:	vmovaps 0x3c43(%rip),%ymm1        # 6020 <_fini+0x6ac>
    23dd:	vmovaps 0x3c5b(%rip),%ymm2        # 6040 <_fini+0x6cc>
    23e5:	vmovaps 0x3c73(%rip),%ymm3        # 6060 <_fini+0x6ec>
    23ed:	vmovaps 0x3c8b(%rip),%ymm4        # 6080 <_fini+0x70c>
    23f5:	vmovaps 0x3ca3(%rip),%ymm5        # 60a0 <_fini+0x72c>
    23fd:	vmovaps 0x3cbb(%rip),%ymm6        # 60c0 <_fini+0x74c>
    2405:	vmovaps 0x3cd3(%rip),%ymm7        # 60e0 <_fini+0x76c>
    240d:	vmovaps 0x3e69(%rip),%zmm8        # 6280 <_fini+0x90c>
    2417:	vmovaps 0x3e9f(%rip),%zmm9        # 62c0 <_fini+0x94c>
    2421:	mov    $0xcc,%dil
    2424:	vmovaps 0x3ed2(%rip),%zmm10        # 6300 <_fini+0x98c>
    242e:	vmovaps 0x3f08(%rip),%zmm11        # 6340 <_fini+0x9cc>
    2438:	vmovaps 0x3f3e(%rip),%zmm12        # 6380 <_fini+0xa0c>
    2442:	vmovaps 0x3f74(%rip),%zmm13        # 63c0 <_fini+0xa4c>
    244c:	vmovaps 0x3faa(%rip),%zmm14        # 6400 <_fini+0xa8c>
    2456:	vmovaps 0x3fe0(%rip),%zmm15        # 6440 <_fini+0xacc>
    2460:	vmovaps 0x3c96(%rip),%ymm16        # 6100 <_fini+0x78c>
    246a:	vmovaps 0x3cac(%rip),%ymm17        # 6120 <_fini+0x7ac>
    2474:	vmovaps 0x3cc2(%rip),%ymm18        # 6140 <_fini+0x7cc>
    247e:	vmovaps 0x3cd8(%rip),%ymm19        # 6160 <_fini+0x7ec>
    2488:	vmovaps 0x3cee(%rip),%ymm20        # 6180 <_fini+0x80c>
    2492:	vmovaps 0x3d04(%rip),%ymm21        # 61a0 <_fini+0x82c>
    249c:	vmovaps 0x3d1a(%rip),%ymm22        # 61c0 <_fini+0x84c>
    24a6:	vmovaps 0x3d30(%rip),%ymm23        # 61e0 <_fini+0x86c>
    24b0:	mov    %r14,%r8
    24b3:	jmp    24db <main_graph_model+0x13b>
    24b5:	data16 cs nopw 0x0(%rax,%rax,1)
    24c0:	inc    %rsi
    24c3:	add    $0x100,%r8
    24ca:	add    $0x8000,%rdx
    24d1:	cmp    $0xc,%rsi
    24d5:	je     2867 <main_graph_model+0x4c7>
    24db:	mov    %rsi,%rcx
    24de:	shl    $0xf,%rcx
    24e2:	lea    (%rax,%rcx,1),%r9
    24e6:	lea    0x8000(%rax,%rcx,1),%rcx
    24ee:	mov    %rsi,%r10
    24f1:	shl    $0x8,%r10
    24f5:	lea    0x5f500(%r14,%r10,1),%r11
    24fd:	cmp    %r11,%r9
    2500:	setb   %bpl
    2504:	add    %r14,%r10
    2507:	cmp    %rcx,%r10
    250a:	setb   %r11b
    250e:	and    %bpl,%r11b
    2511:	mov    %rdx,%r15
    2514:	mov    %r8,%r12
    2517:	xor    %r13d,%r13d
    251a:	jmp    27a6 <main_graph_model+0x406>
    251f:	nop
    2520:	lea    (%r10,%r13,4),%rcx
    2524:	kxnorb %k0,%k0,%k1
    2528:	vxorps %xmm24,%xmm24,%xmm24
    252e:	vgatherdps (%rcx,%ymm0,1),%ymm24{%k1}
    2535:	kxnorb %k0,%k0,%k1
    2539:	vxorps %xmm25,%xmm25,%xmm25
    253f:	vgatherdps (%rcx,%ymm1,1),%ymm25{%k1}
    2546:	kxnorb %k0,%k0,%k1
    254a:	vxorps %xmm26,%xmm26,%xmm26
    2550:	vgatherdps (%rcx,%ymm2,1),%ymm26{%k1}
    2557:	kxnorb %k0,%k0,%k1
    255b:	vxorps %xmm27,%xmm27,%xmm27
    2561:	vgatherdps (%rcx,%ymm3,1),%ymm27{%k1}
    2568:	kxnorb %k0,%k0,%k1
    256c:	vxorps %xmm28,%xmm28,%xmm28
    2572:	vgatherdps (%rcx,%ymm4,1),%ymm28{%k1}
    2579:	kxnorb %k0,%k0,%k1
    257d:	vxorps %xmm29,%xmm29,%xmm29
    2583:	vgatherdps (%rcx,%ymm5,1),%ymm29{%k1}
    258a:	kxnorb %k0,%k0,%k1
    258e:	vxorps %xmm30,%xmm30,%xmm30
    2594:	vgatherdps (%rcx,%ymm6,1),%ymm30{%k1}
    259b:	kxnorb %k0,%k0,%k1
    259f:	vxorps %xmm31,%xmm31,%xmm31
    25a5:	vgatherdps (%rcx,%ymm7,1),%ymm31{%k1}
    25ac:	mov    %r13,%rbp
    25af:	shl    $0x9,%rbp
    25b3:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm24
    25ba:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm25
    25c1:	vinsertf64x4 $0x1,%ymm29,%zmm28,%zmm26
    25c8:	vinsertf64x4 $0x1,%ymm31,%zmm30,%zmm27
    25cf:	vmovaps %zmm26,%zmm28
    25d5:	vpermt2ps %zmm27,%zmm8,%zmm28
    25db:	vmovaps %zmm24,%zmm29
    25e1:	vpermt2ps %zmm25,%zmm9,%zmm29
    25e7:	kmovd  %edi,%k1
    25eb:	vmovapd %zmm28,%zmm29{%k1}
    25f1:	vmovaps %zmm26,%zmm28
    25f7:	vpermt2ps %zmm27,%zmm10,%zmm28
    25fd:	vmovaps %zmm26,%zmm30
    2603:	vpermt2ps %zmm27,%zmm12,%zmm30
    2609:	vmovaps %zmm24,%zmm31
    260f:	vpermt2ps %zmm25,%zmm13,%zmm31
    2615:	vmovapd %zmm30,%zmm31{%k1}
    261b:	vmovaps %zmm24,%zmm30
    2621:	vpermt2ps %zmm25,%zmm11,%zmm30
    2627:	vpermt2ps %zmm27,%zmm14,%zmm26
    262d:	vpermt2ps %zmm25,%zmm15,%zmm24
    2633:	vmovapd %zmm26,%zmm24{%k1}
    2639:	vmovupd %zmm24,0xc0(%r9,%rbp,1)
    2641:	vmovupd %zmm31,0x80(%r9,%rbp,1)
    2649:	vmovapd %zmm28,%zmm30{%k1}
    264f:	vmovupd %zmm30,0x40(%r9,%rbp,1)
    2657:	vmovupd %zmm29,(%r9,%rbp,1)
    265e:	kxnorb %k0,%k0,%k2
    2662:	vxorpd %xmm24,%xmm24,%xmm24
    2668:	vgatherdps (%rcx,%ymm16,1),%ymm24{%k2}
    266f:	kxnorb %k0,%k0,%k2
    2673:	vxorps %xmm25,%xmm25,%xmm25
    2679:	vgatherdps (%rcx,%ymm17,1),%ymm25{%k2}
    2680:	kxnorb %k0,%k0,%k2
    2684:	vxorps %xmm26,%xmm26,%xmm26
    268a:	vgatherdps (%rcx,%ymm18,1),%ymm26{%k2}
    2691:	kxnorb %k0,%k0,%k2
    2695:	vxorps %xmm27,%xmm27,%xmm27
    269b:	vgatherdps (%rcx,%ymm19,1),%ymm27{%k2}
    26a2:	kxnorb %k0,%k0,%k2
    26a6:	vxorps %xmm28,%xmm28,%xmm28
    26ac:	vgatherdps (%rcx,%ymm20,1),%ymm28{%k2}
    26b3:	kxnorb %k0,%k0,%k2
    26b7:	vxorpd %xmm29,%xmm29,%xmm29
    26bd:	vgatherdps (%rcx,%ymm21,1),%ymm29{%k2}
    26c4:	kxnorb %k0,%k0,%k2
    26c8:	vxorpd %xmm30,%xmm30,%xmm30
    26ce:	vgatherdps (%rcx,%ymm22,1),%ymm30{%k2}
    26d5:	kxnorb %k0,%k0,%k2
    26d9:	vxorpd %xmm31,%xmm31,%xmm31
    26df:	vgatherdps (%rcx,%ymm23,1),%ymm31{%k2}
    26e6:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm24
    26ed:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm25
    26f4:	vinsertf64x4 $0x1,%ymm29,%zmm28,%zmm26
    26fb:	vinsertf64x4 $0x1,%ymm31,%zmm30,%zmm27
    2702:	vmovaps %zmm26,%zmm28
    2708:	vpermt2ps %zmm27,%zmm8,%zmm28
    270e:	vmovaps %zmm24,%zmm29
    2714:	vpermt2ps %zmm25,%zmm9,%zmm29
    271a:	vmovapd %zmm28,%zmm29{%k1}
    2720:	vmovaps %zmm26,%zmm28
    2726:	vpermt2ps %zmm27,%zmm10,%zmm28
    272c:	vmovaps %zmm24,%zmm30
    2732:	vpermt2ps %zmm25,%zmm11,%zmm30
    2738:	vmovapd %zmm28,%zmm30{%k1}
    273e:	vmovaps %zmm26,%zmm28
    2744:	vpermt2ps %zmm27,%zmm12,%zmm28
    274a:	vmovaps %zmm24,%zmm31
    2750:	vpermt2ps %zmm25,%zmm13,%zmm31
    2756:	vmovapd %zmm28,%zmm31{%k1}
    275c:	vpermt2ps %zmm27,%zmm14,%zmm26
    2762:	vpermt2ps %zmm25,%zmm15,%zmm24
    2768:	vmovapd %zmm26,%zmm24{%k1}
    276e:	vmovupd %zmm24,0x1c0(%r9,%rbp,1)
    2776:	vmovupd %zmm31,0x180(%r9,%rbp,1)
    277e:	vmovupd %zmm30,0x140(%r9,%rbp,1)
    2786:	vmovupd %zmm29,0x100(%r9,%rbp,1)
    278e:	inc    %r13
    2791:	add    $0x4,%r12
    2795:	add    $0x200,%r15
    279c:	cmp    $0x40,%r13
    27a0:	je     24c0 <main_graph_model+0x120>
    27a6:	test   %r11b,%r11b
    27a9:	je     2520 <main_graph_model+0x180>
    27af:	mov    $0xfffffffffffffff8,%rcx
    27b6:	mov    %r12,%rbp
    27b9:	nopl   0x0(%rax)
    27c0:	vmovss 0x0(%rbp),%xmm24
    27c7:	vmovss %xmm24,0x4(%r15,%rcx,4)
    27cf:	vmovss 0xc00(%rbp),%xmm24
    27d9:	vmovss %xmm24,0x8(%r15,%rcx,4)
    27e1:	vmovss 0x1800(%rbp),%xmm24
    27eb:	vmovss %xmm24,0xc(%r15,%rcx,4)
    27f3:	vmovss 0x2400(%rbp),%xmm24
    27fd:	vmovss %xmm24,0x10(%r15,%rcx,4)
    2805:	vmovss 0x3000(%rbp),%xmm24
    280f:	vmovss %xmm24,0x14(%r15,%rcx,4)
    2817:	vmovss 0x3c00(%rbp),%xmm24
    2821:	vmovss %xmm24,0x18(%r15,%rcx,4)
    2829:	vmovss 0x4800(%rbp),%xmm24
    2833:	vmovss %xmm24,0x1c(%r15,%rcx,4)
    283b:	vmovss 0x5400(%rbp),%xmm24
    2845:	vmovss %xmm24,0x20(%r15,%rcx,4)
    284d:	add    $0x8,%rcx
    2851:	add    $0x6000,%rbp
    2858:	cmp    $0x78,%rcx
    285c:	jb     27c0 <main_graph_model+0x420>
    2862:	jmp    278e <main_graph_model+0x3ee>
    2867:	vmovaps 0x3991(%rip),%ymm0        # 6200 <_fini+0x88c>
    286f:	vmovups %ymm0,0x38(%rbx)
    2874:	vmovaps 0x39a4(%rip),%ymm0        # 6220 <_fini+0x8ac>
    287c:	vmovups %ymm0,0x18(%rbx)
    2881:	mov    %rax,0x8(%rbx)
    2885:	mov    (%rsp),%rax
    2889:	mov    %rax,(%rbx)
    288c:	movq   $0x0,0x10(%rbx)
    2894:	mov    %rbx,%rax
    2897:	add    $0x8,%rsp
    289b:	pop    %rbx
    289c:	pop    %r12
    289e:	pop    %r13
    28a0:	pop    %r14
    28a2:	pop    %r15
    28a4:	pop    %rbp
    28a5:	vzeroupper
    28a8:	ret
    28a9:	nopl   0x0(%rax)
## run_main_graph
    3030:	jmp    2070 <run_main_graph_model@plt>
    3035:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    2dc0:	push   %rbp
    2dc1:	mov    %rsp,%rbp
    2dc4:	push   %r15
    2dc6:	push   %r14
    2dc8:	push   %r13
    2dca:	push   %r12
    2dcc:	push   %rbx
    2dcd:	sub    $0x48,%rsp
    2dd1:	mov    %rdi,%rbx
    2dd4:	call   22a0 <omTensorListGetSize@plt>
    2dd9:	cmp    $0x1,%rax
    2ddd:	jne    2fb1 <run_main_graph_model+0x1f1>
    2de3:	mov    %rbx,%rdi
    2de6:	call   2140 <omTensorListGetOmtArray@plt>
    2deb:	mov    (%rax),%r14
    2dee:	mov    %r14,%rdi
    2df1:	call   22b0 <omTensorGetDataType@plt>
    2df6:	cmp    $0x1,%rax
    2dfa:	jne    2fc6 <run_main_graph_model+0x206>
    2e00:	mov    %r14,%rdi
    2e03:	call   20f0 <omTensorGetRank@plt>
    2e08:	cmp    $0x4,%rax
    2e0c:	jne    2ff3 <run_main_graph_model+0x233>
    2e12:	mov    %r14,%rdi
    2e15:	call   20e0 <omTensorGetShape@plt>
    2e1a:	mov    (%rax),%rsi
    2e1d:	cmp    $0x1,%rsi
    2e21:	jne    2ffc <run_main_graph_model+0x23c>
    2e27:	mov    0x8(%rax),%rsi
    2e2b:	cmp    $0x80,%rsi
    2e32:	jne    3005 <run_main_graph_model+0x245>
    2e38:	mov    0x10(%rax),%rsi
    2e3c:	cmp    $0xc,%rsi
    2e40:	jne    300e <run_main_graph_model+0x24e>
    2e46:	mov    0x18(%rax),%rsi
    2e4a:	cmp    $0x40,%rsi
    2e4e:	jne    3017 <run_main_graph_model+0x257>
    2e54:	mov    %rbx,%rdi
    2e57:	call   2140 <omTensorListGetOmtArray@plt>
    2e5c:	mov    %rsp,%rbx
    2e5f:	lea    -0x60(%rbx),%rcx
    2e63:	mov    %rcx,%rsp
    2e66:	mov    (%rax),%r14
    2e69:	mov    %rsp,%r15
    2e6c:	lea    -0x60(%r15),%rax
    2e70:	mov    %rax,-0x30(%rbp)
    2e74:	mov    %rax,%rsp
    2e77:	mov    %r14,%rdi
    2e7a:	call   20a0 <omTensorGetDataPtr@plt>
    2e7f:	mov    %rax,%r12
    2e82:	mov    %r14,%rdi
    2e85:	call   20e0 <omTensorGetShape@plt>
    2e8a:	mov    %rax,%r13
    2e8d:	mov    %r14,%rdi
    2e90:	call   2120 <omTensorGetStrides@plt>
    2e95:	mov    %r12,-0x60(%r15)
    2e99:	mov    %r12,-0x58(%r15)
    2e9d:	movq   $0x0,-0x50(%r15)
    2ea5:	vmovups 0x0(%r13),%ymm0
    2eab:	vmovups %ymm0,-0x48(%r15)
    2eb1:	vmovups (%rax),%ymm0
    2eb5:	vmovups %ymm0,-0x28(%r15)
    2ebb:	lea    -0x60(%rbx),%rdi
    2ebf:	mov    -0x30(%rbp),%rsi
    2ec3:	vzeroupper
    2ec6:	call   2150 <_mlir_ciface_main_graph_model@plt>
    2ecb:	mov    -0x60(%rbx),%r15
    2ecf:	mov    -0x58(%rbx),%r12
    2ed3:	mov    -0x48(%rbx),%rax
    2ed7:	mov    %rax,-0x58(%rbp)
    2edb:	mov    -0x40(%rbx),%rax
    2edf:	mov    %rax,-0x60(%rbp)
    2ee3:	mov    -0x38(%rbx),%rax
    2ee7:	mov    %rax,-0x68(%rbp)
    2eeb:	mov    -0x30(%rbx),%rax
    2eef:	mov    %rax,-0x30(%rbp)
    2ef3:	mov    -0x28(%rbx),%rax
    2ef7:	mov    %rax,-0x38(%rbp)
    2efb:	mov    -0x20(%rbx),%rax
    2eff:	mov    %rax,-0x40(%rbp)
    2f03:	mov    -0x18(%rbx),%rax
    2f07:	mov    %rax,-0x48(%rbp)
    2f0b:	mov    -0x10(%rbx),%rax
    2f0f:	mov    %rax,-0x50(%rbp)
    2f13:	mov    %rsp,%r13
    2f16:	lea    -0x10(%r13),%rbx
    2f1a:	mov    %rbx,%rsp
    2f1d:	mov    $0x4,%edi
    2f22:	call   2280 <omTensorCreateUntyped@plt>
    2f27:	mov    %rax,%r14
    2f2a:	mov    $0x1,%esi
    2f2f:	mov    %rax,%rdi
    2f32:	mov    %r15,%rdx
    2f35:	mov    %r12,%rcx
    2f38:	call   20d0 <omTensorSetDataPtr@plt>
    2f3d:	mov    $0x1,%esi
    2f42:	mov    %r14,%rdi
    2f45:	call   2290 <omTensorSetDataType@plt>
    2f4a:	mov    %r14,%rdi
    2f4d:	call   20e0 <omTensorGetShape@plt>
    2f52:	mov    %rax,%r15
    2f55:	mov    %r14,%rdi
    2f58:	call   2120 <omTensorGetStrides@plt>
    2f5d:	mov    -0x58(%rbp),%rcx
    2f61:	mov    %rcx,(%r15)
    2f64:	mov    -0x38(%rbp),%rcx
    2f68:	mov    %rcx,(%rax)
    2f6b:	mov    -0x60(%rbp),%rcx
    2f6f:	mov    %rcx,0x8(%r15)
    2f73:	mov    -0x40(%rbp),%rcx
    2f77:	mov    %rcx,0x8(%rax)
    2f7b:	mov    -0x68(%rbp),%rcx
    2f7f:	mov    %rcx,0x10(%r15)
    2f83:	mov    -0x48(%rbp),%rcx
    2f87:	mov    %rcx,0x10(%rax)
    2f8b:	mov    -0x30(%rbp),%rcx
    2f8f:	mov    %rcx,0x18(%r15)
    2f93:	mov    -0x50(%rbp),%rcx
    2f97:	mov    %rcx,0x18(%rax)
    2f9b:	mov    %r14,-0x10(%r13)
    2f9f:	mov    $0x1,%esi
    2fa4:	mov    %rbx,%rdi
    2fa7:	call   21c0 <omTensorListCreate@plt>
    2fac:	mov    %rax,%rbx
    2faf:	jmp    2fe1 <run_main_graph_model+0x221>
    2fb1:	lea    0x3958(%rip),%rdi        # 6910 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    2fb8:	xor    %ebx,%ebx
    2fba:	mov    %rax,%rsi
    2fbd:	xor    %eax,%eax
    2fbf:	call   2040 <printf@plt>
    2fc4:	jmp    2fd6 <run_main_graph_model+0x216>
    2fc6:	lea    0x3913(%rip),%rdi        # 68e0 <om_Wrong data type for the input 0: expect f32^J_model>
    2fcd:	xor    %ebx,%ebx
    2fcf:	xor    %eax,%eax
    2fd1:	call   2040 <printf@plt>
    2fd6:	call   2030 <__errno_location@plt>
    2fdb:	movl   $0x16,(%rax)
    2fe1:	mov    %rbx,%rax
    2fe4:	lea    -0x28(%rbp),%rsp
    2fe8:	pop    %rbx
    2fe9:	pop    %r12
    2feb:	pop    %r13
    2fed:	pop    %r14
    2fef:	pop    %r15
    2ff1:	pop    %rbp
    2ff2:	ret
    2ff3:	lea    0x38a6(%rip),%rdi        # 68a0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    2ffa:	jmp    2fb8 <run_main_graph_model+0x1f8>
    2ffc:	lea    0x384d(%rip),%rdi        # 6850 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    3003:	jmp    301e <run_main_graph_model+0x25e>
    3005:	lea    0x37f4(%rip),%rdi        # 6800 <om_Wrong size for the dimension 1 of the input 0: expect 128, but got %lld^J_model>
    300c:	jmp    301e <run_main_graph_model+0x25e>
    300e:	lea    0x379b(%rip),%rdi        # 67b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    3015:	jmp    301e <run_main_graph_model+0x25e>
    3017:	lea    0x3742(%rip),%rdi        # 6760 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    301e:	xor    %ebx,%ebx
    3020:	jmp    2fbd <run_main_graph_model+0x1fd>
    3022:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x6260>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
