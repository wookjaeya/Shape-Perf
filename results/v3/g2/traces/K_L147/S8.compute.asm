## _mlir_ciface_main_graph_model
    2890:	push   %rbp
    2891:	push   %r15
    2893:	push   %r14
    2895:	push   %r13
    2897:	push   %r12
    2899:	push   %rbx
    289a:	sub    $0x28,%rsp
    289e:	mov    %rdi,0x10(%rsp)
    28a3:	mov    0x8(%rsi),%rbx
    28a7:	mov    $0x6e410,%edi
    28ac:	call   21d0 <malloc@plt>
    28b1:	mov    %rax,0x8(%rsp)
    28b6:	add    $0xf,%rax
    28ba:	and    $0xfffffffffffffff0,%rax
    28be:	mov    %rbx,0x18(%rsp)
    28c3:	lea    0x2400(%rbx),%r13
    28ca:	mov    $0xc,%esi
    28cf:	xor    %edi,%edi
    28d1:	vmovaps 0x3725(%rip),%ymm22        # 6000 <_fini+0x6cc>
    28db:	vmovaps 0x373b(%rip),%ymm29        # 6020 <_fini+0x6ec>
    28e5:	vmovaps 0x3751(%rip),%ymm30        # 6040 <_fini+0x70c>
    28ef:	vmovaps 0x3769(%rip),%ymm3        # 6060 <_fini+0x72c>
    28f7:	vmovaps 0x3781(%rip),%ymm4        # 6080 <_fini+0x74c>
    28ff:	vmovaps 0x3799(%rip),%ymm5        # 60a0 <_fini+0x76c>
    2907:	vmovaps 0x37b1(%rip),%ymm6        # 60c0 <_fini+0x78c>
    290f:	vbroadcastf128 0x40a8(%rip),%ymm7        # 69c0 <_entry_point_0_out_sig_model+0x50>
    2918:	vmovaps 0x3ade(%rip),%zmm8        # 6400 <_fini+0xacc>
    2922:	vmovaps 0x3b14(%rip),%zmm9        # 6440 <_fini+0xb0c>
    292c:	mov    $0x3870,%r8w
    2931:	vmovaps 0x3b45(%rip),%zmm10        # 6480 <_fini+0xb4c>
    293b:	vmovaps 0x3b7b(%rip),%zmm11        # 64c0 <_fini+0xb8c>
    2945:	mov    $0xe1c,%r9w
    294a:	vmovaps 0x3bac(%rip),%zmm12        # 6500 <_fini+0xbcc>
    2954:	vmovaps 0x3be2(%rip),%zmm13        # 6540 <_fini+0xc0c>
    295e:	mov    $0x3c78,%r10w
    2963:	vbroadcastf128 0x4064(%rip),%ymm14        # 69d0 <_entry_point_0_out_sig_model+0x60>
    296c:	vmovaps 0x37ac(%rip),%ymm15        # 6120 <_fini+0x7ec>
    2974:	vmovaps 0x37c2(%rip),%ymm16        # 6140 <_fini+0x80c>
    297e:	vmovaps 0x37d8(%rip),%ymm17        # 6160 <_fini+0x82c>
    2988:	vmovaps 0x37ee(%rip),%ymm18        # 6180 <_fini+0x84c>
    2992:	vmovaps 0x3804(%rip),%ymm19        # 61a0 <_fini+0x86c>
    299c:	vmovaps 0x381a(%rip),%ymm20        # 61c0 <_fini+0x88c>
    29a6:	vmovaps 0x3830(%rip),%ymm21        # 61e0 <_fini+0x8ac>
    29b0:	imul   $0x9300,%rdi,%rcx
    29b7:	lea    (%rax,%rcx,1),%r11
    29bb:	lea    0x9300(%rax,%rcx,1),%rcx
    29c3:	mov    %rdi,%rdx
    29c6:	shl    $0x8,%rdx
    29ca:	mov    0x18(%rsp),%r14
    29cf:	lea    0x6d900(%r14,%rdx,1),%rbx
    29d7:	cmp    %rbx,%r11
    29da:	setb   %bl
    29dd:	lea    (%r14,%rdx,1),%r15
    29e1:	cmp    %rcx,%r15
    29e4:	setb   %bpl
    29e8:	and    %bl,%bpl
    29eb:	mov    %rsi,%r12
    29ee:	mov    %r13,0x20(%rsp)
    29f3:	xor    %ecx,%ecx
    29f5:	data16 cs nopw 0x0(%rax,%rax,1)
    2a00:	test   %bpl,%bpl
    2a03:	je     2a10 <_mlir_ciface_main_graph_model+0x180>
    2a05:	xor    %ebx,%ebx
    2a07:	jmp    2c65 <_mlir_ciface_main_graph_model+0x3d5>
    2a0c:	nopl   0x0(%rax)
    2a10:	lea    (%r15,%rcx,4),%r14
    2a14:	vxorps %xmm23,%xmm23,%xmm23
    2a1a:	kxnorb %k0,%k0,%k1
    2a1e:	vgatherdps (%r14,%ymm22,1),%ymm23{%k1}
    2a25:	vxorps %xmm24,%xmm24,%xmm24
    2a2b:	kxnorb %k0,%k0,%k1
    2a2f:	vgatherdps (%r14,%ymm29,1),%ymm24{%k1}
    2a36:	vxorps %xmm25,%xmm25,%xmm25
    2a3c:	kxnorb %k0,%k0,%k1
    2a40:	vgatherdps (%r14,%ymm30,1),%ymm25{%k1}
    2a47:	vxorps %xmm26,%xmm26,%xmm26
    2a4d:	kxnorb %k0,%k0,%k1
    2a51:	vgatherdps (%r14,%ymm3,1),%ymm26{%k1}
    2a58:	vxorps %xmm27,%xmm27,%xmm27
    2a5e:	kxnorb %k0,%k0,%k1
    2a62:	vgatherdps (%r14,%ymm4,1),%ymm27{%k1}
    2a69:	vxorps %xmm28,%xmm28,%xmm28
    2a6f:	kxnorb %k0,%k0,%k1
    2a73:	vgatherdps (%r14,%ymm5,1),%ymm28{%k1}
    2a7a:	imul   $0x24c,%rcx,%rbx
    2a81:	vxorps %xmm0,%xmm0,%xmm0
    2a85:	kxnorb %k0,%k0,%k1
    2a89:	vgatherdps (%r14,%ymm6,1),%ymm0{%k1}
    2a90:	vinsertf64x4 $0x1,%ymm24,%zmm23,%zmm23
    2a97:	vinsertf64x4 $0x1,%ymm26,%zmm25,%zmm24
    2a9e:	vinsertf64x4 $0x1,%ymm28,%zmm27,%zmm25
    2aa5:	vmovaps %zmm23,%zmm1
    2aab:	vpermt2ps %zmm24,%zmm7,%zmm1
    2ab1:	vmovaps %zmm23,%zmm26
    2ab7:	vpermt2ps %zmm24,%zmm8,%zmm26
    2abd:	vmovaps %zmm25,%zmm27
    2ac3:	vpermt2ps %zmm0,%zmm9,%zmm27
    2ac9:	kmovd  %r8d,%k1
    2ace:	vmovaps %zmm27,%zmm26{%k1}
    2ad4:	vmovaps %zmm24,%zmm27
    2ada:	vpermt2ps %zmm23,%zmm10,%zmm27
    2ae0:	vmovaps %zmm25,%zmm28
    2ae6:	vpermt2ps %zmm0,%zmm11,%zmm28
    2aec:	kmovd  %r9d,%k2
    2af1:	vmovaps %zmm28,%zmm27{%k2}
    2af7:	vpermt2ps %zmm24,%zmm12,%zmm23
    2afd:	vmovaps %zmm25,%zmm24
    2b03:	vpermt2ps %zmm0,%zmm13,%zmm24
    2b09:	kmovd  %r10d,%k3
    2b0e:	vmovaps %zmm23,%zmm24{%k3}
    2b14:	vpermt2ps %zmm25,%zmm14,%zmm0
    2b1a:	vmovups %zmm24,0x80(%r11,%rbx,1)
    2b22:	vmovups %zmm27,0x40(%r11,%rbx,1)
    2b2a:	vmovups %zmm26,(%r11,%rbx,1)
    2b31:	vblendps $0xe1,%ymm0,%ymm1,%ymm0
    2b37:	vmovups %ymm0,0xc0(%r11,%rbx,1)
    2b41:	vxorps %xmm1,%xmm1,%xmm1
    2b45:	kxnorb %k0,%k0,%k4
    2b49:	vgatherdps (%r14,%ymm15,1),%ymm1{%k4}
    2b50:	vxorps %xmm23,%xmm23,%xmm23
    2b56:	kxnorb %k0,%k0,%k4
    2b5a:	vgatherdps (%r14,%ymm16,1),%ymm23{%k4}
    2b61:	vxorps %xmm24,%xmm24,%xmm24
    2b67:	kxnorb %k0,%k0,%k4
    2b6b:	vgatherdps (%r14,%ymm17,1),%ymm24{%k4}
    2b72:	vxorps %xmm25,%xmm25,%xmm25
    2b78:	kxnorb %k0,%k0,%k4
    2b7c:	vgatherdps (%r14,%ymm18,1),%ymm25{%k4}
    2b83:	vxorps %xmm26,%xmm26,%xmm26
    2b89:	kxnorb %k0,%k0,%k4
    2b8d:	vgatherdps (%r14,%ymm19,1),%ymm26{%k4}
    2b94:	vxorps %xmm27,%xmm27,%xmm27
    2b9a:	kxnorb %k0,%k0,%k4
    2b9e:	vgatherdps (%r14,%ymm20,1),%ymm27{%k4}
    2ba5:	vxorps %xmm0,%xmm0,%xmm0
    2ba9:	kxnorb %k0,%k0,%k4
    2bad:	vgatherdps (%r14,%ymm21,1),%ymm0{%k4}
    2bb4:	vinsertf64x4 $0x1,%ymm23,%zmm1,%zmm1
    2bbb:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm23
    2bc2:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm24
    2bc9:	vmovaps %zmm1,%zmm2
    2bcf:	vpermt2ps %zmm23,%zmm7,%zmm2
    2bd5:	vmovaps %zmm1,%zmm25
    2bdb:	vpermt2ps %zmm23,%zmm8,%zmm25
    2be1:	vmovaps %zmm24,%zmm26
    2be7:	vpermt2ps %zmm0,%zmm9,%zmm26
    2bed:	vmovaps %zmm26,%zmm25{%k1}
    2bf3:	vmovaps %zmm23,%zmm26
    2bf9:	vpermt2ps %zmm1,%zmm10,%zmm26
    2bff:	vmovaps %zmm24,%zmm27
    2c05:	vpermt2ps %zmm0,%zmm11,%zmm27
    2c0b:	vmovaps %zmm27,%zmm26{%k2}
    2c11:	vpermt2ps %zmm23,%zmm12,%zmm1
    2c17:	vmovaps %zmm24,%zmm23
    2c1d:	vpermt2ps %zmm0,%zmm13,%zmm23
    2c23:	vmovaps %zmm1,%zmm23{%k3}
    2c29:	vpermt2ps %zmm24,%zmm14,%zmm0
    2c2f:	vmovups %zmm23,0x160(%r11,%rbx,1)
    2c3a:	vmovups %zmm26,0x120(%r11,%rbx,1)
    2c45:	vmovups %zmm25,0xe0(%r11,%rbx,1)
    2c50:	vblendps $0xe1,%ymm0,%ymm2,%ymm0
    2c56:	vmovups %ymm0,0x1a0(%r11,%rbx,1)
    2c60:	mov    $0x70,%ebx
    2c65:	lea    (%rbx,%rbx,2),%r14
    2c69:	add    $0xfffffffffffffff9,%rbx
    2c6d:	shl    $0xa,%r14d
    2c71:	add    %r13,%r14
    2c74:	lea    (%rax,%r12,1),%rdx
    2c78:	nopl   0x0(%rax,%rax,1)
    2c80:	vmovss -0x2400(%r14),%xmm0
    2c89:	vmovss %xmm0,0x10(%rdx,%rbx,4)
    2c8f:	vmovss -0x1800(%r14),%xmm0
    2c98:	vmovss %xmm0,0x14(%rdx,%rbx,4)
    2c9e:	vmovss -0xc00(%r14),%xmm0
    2ca7:	vmovss %xmm0,0x18(%rdx,%rbx,4)
    2cad:	vmovss (%r14),%xmm0
    2cb2:	vmovss %xmm0,0x1c(%rdx,%rbx,4)
    2cb8:	vmovss 0xc00(%r14),%xmm0
    2cc1:	vmovss %xmm0,0x20(%rdx,%rbx,4)
    2cc7:	vmovss 0x1800(%r14),%xmm0
    2cd0:	vmovss %xmm0,0x24(%rdx,%rbx,4)
    2cd6:	vmovss 0x2400(%r14),%xmm0
    2cdf:	vmovss %xmm0,0x28(%rdx,%rbx,4)
    2ce5:	add    $0x7,%rbx
    2ce9:	add    $0x5400,%r14
    2cf0:	cmp    $0x8c,%rbx
    2cf7:	jb     2c80 <_mlir_ciface_main_graph_model+0x3f0>
    2cf9:	inc    %rcx
    2cfc:	add    $0x4,%r13
    2d00:	add    $0x24c,%r12
    2d07:	cmp    $0x40,%rcx
    2d0b:	jne    2a00 <_mlir_ciface_main_graph_model+0x170>
    2d11:	inc    %rdi
    2d14:	mov    0x20(%rsp),%r13
    2d19:	add    $0x100,%r13
    2d20:	add    $0x9300,%rsi
    2d27:	cmp    $0xc,%rdi
    2d2b:	jne    29b0 <_mlir_ciface_main_graph_model+0x120>
    2d31:	mov    0x10(%rsp),%rcx
    2d36:	mov    0x8(%rsp),%rdx
    2d3b:	mov    %rdx,(%rcx)
    2d3e:	mov    %rax,0x8(%rcx)
    2d42:	vmovaps 0x34f6(%rip),%ymm0        # 6240 <_fini+0x90c>
    2d4a:	vmovups %ymm0,0x10(%rcx)
    2d4f:	vmovaps 0x3509(%rip),%ymm0        # 6260 <_fini+0x92c>
    2d57:	vmovups %ymm0,0x30(%rcx)
    2d5c:	movq   $0x1,0x50(%rcx)
    2d64:	add    $0x28,%rsp
    2d68:	pop    %rbx
    2d69:	pop    %r12
    2d6b:	pop    %r13
    2d6d:	pop    %r14
    2d6f:	pop    %r15
    2d71:	pop    %rbp
    2d72:	vzeroupper
    2d75:	ret
    2d76:	cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2150:	jmp    *0x6f3a(%rip)        # 9090 <_mlir_ciface_main_graph_model@@Base+0x6800>
    2156:	push   $0x12
    215b:	jmp    2020 <_init+0x20>
## main_graph_model
    23a0:	push   %rbp
    23a1:	push   %r15
    23a3:	push   %r14
    23a5:	push   %r13
    23a7:	push   %r12
    23a9:	push   %rbx
    23aa:	sub    $0x28,%rsp
    23ae:	mov    %rdx,%rbx
    23b1:	mov    %rdi,0x10(%rsp)
    23b6:	mov    $0x6e410,%edi
    23bb:	call   21d0 <malloc@plt>
    23c0:	mov    %rax,0x8(%rsp)
    23c5:	add    $0xf,%rax
    23c9:	and    $0xfffffffffffffff0,%rax
    23cd:	mov    %rbx,0x18(%rsp)
    23d2:	lea    0x2400(%rbx),%r13
    23d9:	mov    $0xc,%esi
    23de:	xor    %edi,%edi
    23e0:	vmovaps 0x3c16(%rip),%ymm22        # 6000 <_fini+0x6cc>
    23ea:	vmovaps 0x3c2c(%rip),%ymm29        # 6020 <_fini+0x6ec>
    23f4:	vmovaps 0x3c42(%rip),%ymm30        # 6040 <_fini+0x70c>
    23fe:	vmovaps 0x3c5a(%rip),%ymm3        # 6060 <_fini+0x72c>
    2406:	vmovaps 0x3c72(%rip),%ymm4        # 6080 <_fini+0x74c>
    240e:	vmovaps 0x3c8a(%rip),%ymm5        # 60a0 <_fini+0x76c>
    2416:	vmovaps 0x3ca2(%rip),%ymm6        # 60c0 <_fini+0x78c>
    241e:	vbroadcastf128 0x4599(%rip),%ymm7        # 69c0 <_entry_point_0_out_sig_model+0x50>
    2427:	vmovaps 0x3e4f(%rip),%zmm8        # 6280 <_fini+0x94c>
    2431:	vmovaps 0x3e85(%rip),%zmm9        # 62c0 <_fini+0x98c>
    243b:	mov    $0x3870,%r8w
    2440:	vmovaps 0x3eb6(%rip),%zmm10        # 6300 <_fini+0x9cc>
    244a:	vmovaps 0x3eec(%rip),%zmm11        # 6340 <_fini+0xa0c>
    2454:	mov    $0xe1c,%r9w
    2459:	vmovaps 0x3f1d(%rip),%zmm12        # 6380 <_fini+0xa4c>
    2463:	vmovaps 0x3f53(%rip),%zmm13        # 63c0 <_fini+0xa8c>
    246d:	mov    $0x3c78,%r10w
    2472:	vbroadcastf128 0x4555(%rip),%ymm14        # 69d0 <_entry_point_0_out_sig_model+0x60>
    247b:	vmovaps 0x3c9d(%rip),%ymm15        # 6120 <_fini+0x7ec>
    2483:	vmovaps 0x3cb3(%rip),%ymm16        # 6140 <_fini+0x80c>
    248d:	vmovaps 0x3cc9(%rip),%ymm17        # 6160 <_fini+0x82c>
    2497:	vmovaps 0x3cdf(%rip),%ymm18        # 6180 <_fini+0x84c>
    24a1:	vmovaps 0x3cf5(%rip),%ymm19        # 61a0 <_fini+0x86c>
    24ab:	vmovaps 0x3d0b(%rip),%ymm20        # 61c0 <_fini+0x88c>
    24b5:	vmovaps 0x3d21(%rip),%ymm21        # 61e0 <_fini+0x8ac>
    24bf:	nop
    24c0:	imul   $0x9300,%rdi,%rcx
    24c7:	lea    (%rax,%rcx,1),%r11
    24cb:	lea    0x9300(%rax,%rcx,1),%rcx
    24d3:	mov    %rdi,%rdx
    24d6:	shl    $0x8,%rdx
    24da:	mov    0x18(%rsp),%r14
    24df:	lea    0x6d900(%r14,%rdx,1),%rbx
    24e7:	cmp    %rbx,%r11
    24ea:	setb   %bl
    24ed:	lea    (%r14,%rdx,1),%r15
    24f1:	cmp    %rcx,%r15
    24f4:	setb   %bpl
    24f8:	and    %bl,%bpl
    24fb:	mov    %rsi,%r12
    24fe:	mov    %r13,0x20(%rsp)
    2503:	xor    %ecx,%ecx
    2505:	data16 cs nopw 0x0(%rax,%rax,1)
    2510:	test   %bpl,%bpl
    2513:	je     2520 <main_graph_model+0x180>
    2515:	xor    %ebx,%ebx
    2517:	jmp    2775 <main_graph_model+0x3d5>
    251c:	nopl   0x0(%rax)
    2520:	lea    (%r15,%rcx,4),%r14
    2524:	kxnorb %k0,%k0,%k1
    2528:	vxorps %xmm23,%xmm23,%xmm23
    252e:	vgatherdps (%r14,%ymm22,1),%ymm23{%k1}
    2535:	kxnorb %k0,%k0,%k1
    2539:	vxorps %xmm24,%xmm24,%xmm24
    253f:	vgatherdps (%r14,%ymm29,1),%ymm24{%k1}
    2546:	kxnorb %k0,%k0,%k1
    254a:	vxorps %xmm25,%xmm25,%xmm25
    2550:	vgatherdps (%r14,%ymm30,1),%ymm25{%k1}
    2557:	kxnorb %k0,%k0,%k1
    255b:	vxorps %xmm26,%xmm26,%xmm26
    2561:	vgatherdps (%r14,%ymm3,1),%ymm26{%k1}
    2568:	kxnorb %k0,%k0,%k1
    256c:	vxorps %xmm27,%xmm27,%xmm27
    2572:	vgatherdps (%r14,%ymm4,1),%ymm27{%k1}
    2579:	kxnorb %k0,%k0,%k1
    257d:	vxorps %xmm28,%xmm28,%xmm28
    2583:	vgatherdps (%r14,%ymm5,1),%ymm28{%k1}
    258a:	imul   $0x24c,%rcx,%rbx
    2591:	kxnorb %k0,%k0,%k1
    2595:	vxorps %xmm0,%xmm0,%xmm0
    2599:	vgatherdps (%r14,%ymm6,1),%ymm0{%k1}
    25a0:	vinsertf64x4 $0x1,%ymm24,%zmm23,%zmm23
    25a7:	vinsertf64x4 $0x1,%ymm26,%zmm25,%zmm24
    25ae:	vinsertf64x4 $0x1,%ymm28,%zmm27,%zmm25
    25b5:	vmovaps %zmm23,%zmm1
    25bb:	vpermt2ps %zmm24,%zmm7,%zmm1
    25c1:	vmovaps %zmm23,%zmm26
    25c7:	vpermt2ps %zmm24,%zmm8,%zmm26
    25cd:	vmovaps %zmm25,%zmm27
    25d3:	vpermt2ps %zmm0,%zmm9,%zmm27
    25d9:	kmovd  %r8d,%k1
    25de:	vmovaps %zmm27,%zmm26{%k1}
    25e4:	vmovaps %zmm24,%zmm27
    25ea:	vpermt2ps %zmm23,%zmm10,%zmm27
    25f0:	vmovaps %zmm25,%zmm28
    25f6:	vpermt2ps %zmm0,%zmm11,%zmm28
    25fc:	kmovd  %r9d,%k2
    2601:	vmovaps %zmm28,%zmm27{%k2}
    2607:	vpermt2ps %zmm24,%zmm12,%zmm23
    260d:	vmovaps %zmm25,%zmm24
    2613:	vpermt2ps %zmm0,%zmm13,%zmm24
    2619:	kmovd  %r10d,%k3
    261e:	vmovaps %zmm23,%zmm24{%k3}
    2624:	vpermt2ps %zmm25,%zmm14,%zmm0
    262a:	vmovups %zmm24,0x80(%r11,%rbx,1)
    2632:	vmovups %zmm27,0x40(%r11,%rbx,1)
    263a:	vmovups %zmm26,(%r11,%rbx,1)
    2641:	vblendps $0xe1,%ymm0,%ymm1,%ymm0
    2647:	vmovups %ymm0,0xc0(%r11,%rbx,1)
    2651:	kxnorb %k0,%k0,%k4
    2655:	vxorps %xmm1,%xmm1,%xmm1
    2659:	vgatherdps (%r14,%ymm15,1),%ymm1{%k4}
    2660:	kxnorb %k0,%k0,%k4
    2664:	vxorps %xmm23,%xmm23,%xmm23
    266a:	vgatherdps (%r14,%ymm16,1),%ymm23{%k4}
    2671:	kxnorb %k0,%k0,%k4
    2675:	vxorps %xmm24,%xmm24,%xmm24
    267b:	vgatherdps (%r14,%ymm17,1),%ymm24{%k4}
    2682:	kxnorb %k0,%k0,%k4
    2686:	vxorps %xmm25,%xmm25,%xmm25
    268c:	vgatherdps (%r14,%ymm18,1),%ymm25{%k4}
    2693:	kxnorb %k0,%k0,%k4
    2697:	vxorps %xmm26,%xmm26,%xmm26
    269d:	vgatherdps (%r14,%ymm19,1),%ymm26{%k4}
    26a4:	kxnorb %k0,%k0,%k4
    26a8:	vxorps %xmm27,%xmm27,%xmm27
    26ae:	vgatherdps (%r14,%ymm20,1),%ymm27{%k4}
    26b5:	kxnorb %k0,%k0,%k4
    26b9:	vxorps %xmm0,%xmm0,%xmm0
    26bd:	vgatherdps (%r14,%ymm21,1),%ymm0{%k4}
    26c4:	vinsertf64x4 $0x1,%ymm23,%zmm1,%zmm1
    26cb:	vinsertf64x4 $0x1,%ymm25,%zmm24,%zmm23
    26d2:	vinsertf64x4 $0x1,%ymm27,%zmm26,%zmm24
    26d9:	vmovaps %zmm1,%zmm2
    26df:	vpermt2ps %zmm23,%zmm7,%zmm2
    26e5:	vmovaps %zmm1,%zmm25
    26eb:	vpermt2ps %zmm23,%zmm8,%zmm25
    26f1:	vmovaps %zmm24,%zmm26
    26f7:	vpermt2ps %zmm0,%zmm9,%zmm26
    26fd:	vmovaps %zmm26,%zmm25{%k1}
    2703:	vmovaps %zmm23,%zmm26
    2709:	vpermt2ps %zmm1,%zmm10,%zmm26
    270f:	vmovaps %zmm24,%zmm27
    2715:	vpermt2ps %zmm0,%zmm11,%zmm27
    271b:	vmovaps %zmm27,%zmm26{%k2}
    2721:	vpermt2ps %zmm23,%zmm12,%zmm1
    2727:	vmovaps %zmm24,%zmm23
    272d:	vpermt2ps %zmm0,%zmm13,%zmm23
    2733:	vmovaps %zmm1,%zmm23{%k3}
    2739:	vpermt2ps %zmm24,%zmm14,%zmm0
    273f:	vmovups %zmm23,0x160(%r11,%rbx,1)
    274a:	vmovups %zmm26,0x120(%r11,%rbx,1)
    2755:	vmovups %zmm25,0xe0(%r11,%rbx,1)
    2760:	vblendps $0xe1,%ymm0,%ymm2,%ymm0
    2766:	vmovups %ymm0,0x1a0(%r11,%rbx,1)
    2770:	mov    $0x70,%ebx
    2775:	lea    (%rbx,%rbx,2),%r14
    2779:	add    $0xfffffffffffffff9,%rbx
    277d:	shl    $0xa,%r14d
    2781:	add    %r13,%r14
    2784:	lea    (%rax,%r12,1),%rdx
    2788:	nopl   0x0(%rax,%rax,1)
    2790:	vmovss -0x2400(%r14),%xmm0
    2799:	vmovss %xmm0,0x10(%rdx,%rbx,4)
    279f:	vmovss -0x1800(%r14),%xmm0
    27a8:	vmovss %xmm0,0x14(%rdx,%rbx,4)
    27ae:	vmovss -0xc00(%r14),%xmm0
    27b7:	vmovss %xmm0,0x18(%rdx,%rbx,4)
    27bd:	vmovss (%r14),%xmm0
    27c2:	vmovss %xmm0,0x1c(%rdx,%rbx,4)
    27c8:	vmovss 0xc00(%r14),%xmm0
    27d1:	vmovss %xmm0,0x20(%rdx,%rbx,4)
    27d7:	vmovss 0x1800(%r14),%xmm0
    27e0:	vmovss %xmm0,0x24(%rdx,%rbx,4)
    27e6:	vmovss 0x2400(%r14),%xmm0
    27ef:	vmovss %xmm0,0x28(%rdx,%rbx,4)
    27f5:	add    $0x7,%rbx
    27f9:	add    $0x5400,%r14
    2800:	cmp    $0x8c,%rbx
    2807:	jb     2790 <main_graph_model+0x3f0>
    2809:	inc    %rcx
    280c:	add    $0x4,%r13
    2810:	add    $0x24c,%r12
    2817:	cmp    $0x40,%rcx
    281b:	jne    2510 <main_graph_model+0x170>
    2821:	inc    %rdi
    2824:	mov    0x20(%rsp),%r13
    2829:	add    $0x100,%r13
    2830:	add    $0x9300,%rsi
    2837:	cmp    $0xc,%rdi
    283b:	jne    24c0 <main_graph_model+0x120>
    2841:	vmovaps 0x39b7(%rip),%ymm0        # 6200 <_fini+0x8cc>
    2849:	mov    0x10(%rsp),%rcx
    284e:	vmovups %ymm0,0x38(%rcx)
    2853:	vmovaps 0x39c5(%rip),%ymm0        # 6220 <_fini+0x8ec>
    285b:	vmovups %ymm0,0x18(%rcx)
    2860:	mov    %rax,0x8(%rcx)
    2864:	mov    0x8(%rsp),%rax
    2869:	mov    %rax,(%rcx)
    286c:	movq   $0x0,0x10(%rcx)
    2874:	mov    %rcx,%rax
    2877:	add    $0x28,%rsp
    287b:	pop    %rbx
    287c:	pop    %r12
    287e:	pop    %r13
    2880:	pop    %r14
    2882:	pop    %r15
    2884:	pop    %rbp
    2885:	vzeroupper
    2888:	ret
    2889:	nopl   0x0(%rax)
## run_main_graph
    2ff0:	jmp    2070 <run_main_graph_model@plt>
    2ff5:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    2d80:	push   %rbp
    2d81:	mov    %rsp,%rbp
    2d84:	push   %r15
    2d86:	push   %r14
    2d88:	push   %r13
    2d8a:	push   %r12
    2d8c:	push   %rbx
    2d8d:	sub    $0x48,%rsp
    2d91:	mov    %rdi,%rbx
    2d94:	call   22a0 <omTensorListGetSize@plt>
    2d99:	cmp    $0x1,%rax
    2d9d:	jne    2f71 <run_main_graph_model+0x1f1>
    2da3:	mov    %rbx,%rdi
    2da6:	call   2140 <omTensorListGetOmtArray@plt>
    2dab:	mov    (%rax),%r14
    2dae:	mov    %r14,%rdi
    2db1:	call   22b0 <omTensorGetDataType@plt>
    2db6:	cmp    $0x1,%rax
    2dba:	jne    2f86 <run_main_graph_model+0x206>
    2dc0:	mov    %r14,%rdi
    2dc3:	call   20f0 <omTensorGetRank@plt>
    2dc8:	cmp    $0x4,%rax
    2dcc:	jne    2fb3 <run_main_graph_model+0x233>
    2dd2:	mov    %r14,%rdi
    2dd5:	call   20e0 <omTensorGetShape@plt>
    2dda:	mov    (%rax),%rsi
    2ddd:	cmp    $0x1,%rsi
    2de1:	jne    2fbc <run_main_graph_model+0x23c>
    2de7:	mov    0x8(%rax),%rsi
    2deb:	cmp    $0x93,%rsi
    2df2:	jne    2fc5 <run_main_graph_model+0x245>
    2df8:	mov    0x10(%rax),%rsi
    2dfc:	cmp    $0xc,%rsi
    2e00:	jne    2fce <run_main_graph_model+0x24e>
    2e06:	mov    0x18(%rax),%rsi
    2e0a:	cmp    $0x40,%rsi
    2e0e:	jne    2fd7 <run_main_graph_model+0x257>
    2e14:	mov    %rbx,%rdi
    2e17:	call   2140 <omTensorListGetOmtArray@plt>
    2e1c:	mov    %rsp,%rbx
    2e1f:	lea    -0x60(%rbx),%rcx
    2e23:	mov    %rcx,%rsp
    2e26:	mov    (%rax),%r14
    2e29:	mov    %rsp,%r15
    2e2c:	lea    -0x60(%r15),%rax
    2e30:	mov    %rax,-0x30(%rbp)
    2e34:	mov    %rax,%rsp
    2e37:	mov    %r14,%rdi
    2e3a:	call   20a0 <omTensorGetDataPtr@plt>
    2e3f:	mov    %rax,%r12
    2e42:	mov    %r14,%rdi
    2e45:	call   20e0 <omTensorGetShape@plt>
    2e4a:	mov    %rax,%r13
    2e4d:	mov    %r14,%rdi
    2e50:	call   2120 <omTensorGetStrides@plt>
    2e55:	mov    %r12,-0x60(%r15)
    2e59:	mov    %r12,-0x58(%r15)
    2e5d:	movq   $0x0,-0x50(%r15)
    2e65:	vmovups 0x0(%r13),%ymm0
    2e6b:	vmovups %ymm0,-0x48(%r15)
    2e71:	vmovups (%rax),%ymm0
    2e75:	vmovups %ymm0,-0x28(%r15)
    2e7b:	lea    -0x60(%rbx),%rdi
    2e7f:	mov    -0x30(%rbp),%rsi
    2e83:	vzeroupper
    2e86:	call   2150 <_mlir_ciface_main_graph_model@plt>
    2e8b:	mov    -0x60(%rbx),%r15
    2e8f:	mov    -0x58(%rbx),%r12
    2e93:	mov    -0x48(%rbx),%rax
    2e97:	mov    %rax,-0x58(%rbp)
    2e9b:	mov    -0x40(%rbx),%rax
    2e9f:	mov    %rax,-0x60(%rbp)
    2ea3:	mov    -0x38(%rbx),%rax
    2ea7:	mov    %rax,-0x68(%rbp)
    2eab:	mov    -0x30(%rbx),%rax
    2eaf:	mov    %rax,-0x30(%rbp)
    2eb3:	mov    -0x28(%rbx),%rax
    2eb7:	mov    %rax,-0x38(%rbp)
    2ebb:	mov    -0x20(%rbx),%rax
    2ebf:	mov    %rax,-0x40(%rbp)
    2ec3:	mov    -0x18(%rbx),%rax
    2ec7:	mov    %rax,-0x48(%rbp)
    2ecb:	mov    -0x10(%rbx),%rax
    2ecf:	mov    %rax,-0x50(%rbp)
    2ed3:	mov    %rsp,%r13
    2ed6:	lea    -0x10(%r13),%rbx
    2eda:	mov    %rbx,%rsp
    2edd:	mov    $0x4,%edi
    2ee2:	call   2280 <omTensorCreateUntyped@plt>
    2ee7:	mov    %rax,%r14
    2eea:	mov    $0x1,%esi
    2eef:	mov    %rax,%rdi
    2ef2:	mov    %r15,%rdx
    2ef5:	mov    %r12,%rcx
    2ef8:	call   20d0 <omTensorSetDataPtr@plt>
    2efd:	mov    $0x1,%esi
    2f02:	mov    %r14,%rdi
    2f05:	call   2290 <omTensorSetDataType@plt>
    2f0a:	mov    %r14,%rdi
    2f0d:	call   20e0 <omTensorGetShape@plt>
    2f12:	mov    %rax,%r15
    2f15:	mov    %r14,%rdi
    2f18:	call   2120 <omTensorGetStrides@plt>
    2f1d:	mov    -0x58(%rbp),%rcx
    2f21:	mov    %rcx,(%r15)
    2f24:	mov    -0x38(%rbp),%rcx
    2f28:	mov    %rcx,(%rax)
    2f2b:	mov    -0x60(%rbp),%rcx
    2f2f:	mov    %rcx,0x8(%r15)
    2f33:	mov    -0x40(%rbp),%rcx
    2f37:	mov    %rcx,0x8(%rax)
    2f3b:	mov    -0x68(%rbp),%rcx
    2f3f:	mov    %rcx,0x10(%r15)
    2f43:	mov    -0x48(%rbp),%rcx
    2f47:	mov    %rcx,0x10(%rax)
    2f4b:	mov    -0x30(%rbp),%rcx
    2f4f:	mov    %rcx,0x18(%r15)
    2f53:	mov    -0x50(%rbp),%rcx
    2f57:	mov    %rcx,0x18(%rax)
    2f5b:	mov    %r14,-0x10(%r13)
    2f5f:	mov    $0x1,%esi
    2f64:	mov    %rbx,%rdi
    2f67:	call   21c0 <omTensorListCreate@plt>
    2f6c:	mov    %rax,%rbx
    2f6f:	jmp    2fa1 <run_main_graph_model+0x221>
    2f71:	lea    0x3898(%rip),%rdi        # 6810 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    2f78:	xor    %ebx,%ebx
    2f7a:	mov    %rax,%rsi
    2f7d:	xor    %eax,%eax
    2f7f:	call   2040 <printf@plt>
    2f84:	jmp    2f96 <run_main_graph_model+0x216>
    2f86:	lea    0x3853(%rip),%rdi        # 67e0 <om_Wrong data type for the input 0: expect f32^J_model>
    2f8d:	xor    %ebx,%ebx
    2f8f:	xor    %eax,%eax
    2f91:	call   2040 <printf@plt>
    2f96:	call   2030 <__errno_location@plt>
    2f9b:	movl   $0x16,(%rax)
    2fa1:	mov    %rbx,%rax
    2fa4:	lea    -0x28(%rbp),%rsp
    2fa8:	pop    %rbx
    2fa9:	pop    %r12
    2fab:	pop    %r13
    2fad:	pop    %r14
    2faf:	pop    %r15
    2fb1:	pop    %rbp
    2fb2:	ret
    2fb3:	lea    0x37e6(%rip),%rdi        # 67a0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    2fba:	jmp    2f78 <run_main_graph_model+0x1f8>
    2fbc:	lea    0x378d(%rip),%rdi        # 6750 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    2fc3:	jmp    2fde <run_main_graph_model+0x25e>
    2fc5:	lea    0x3734(%rip),%rdi        # 6700 <om_Wrong size for the dimension 1 of the input 0: expect 147, but got %lld^J_model>
    2fcc:	jmp    2fde <run_main_graph_model+0x25e>
    2fce:	lea    0x36db(%rip),%rdi        # 66b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    2fd5:	jmp    2fde <run_main_graph_model+0x25e>
    2fd7:	lea    0x3682(%rip),%rdi        # 6660 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    2fde:	xor    %ebx,%ebx
    2fe0:	jmp    2f7d <run_main_graph_model+0x1fd>
    2fe2:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x62a0>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
