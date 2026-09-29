## _mlir_ciface_main_graph_model
    4260:	push   %rbx
    4261:	sub    $0x90,%rsp
    4268:	mov    %rdi,%rbx
    426b:	mov    0x8(%rsi),%rdx
    426f:	lea    0x38(%rsp),%rdi
    4274:	call   2120 <main_graph_model@plt>
    4279:	mov    0x88(%rsp),%rax
    4281:	vmovups 0x78(%rsp),%xmm0
    4287:	vmovups 0x38(%rsp),%ymm1
    428d:	vmovups 0x58(%rsp),%ymm2
    4293:	vmovups %ymm1,(%rbx)
    4297:	vmovups %ymm2,0x20(%rbx)
    429c:	vmovups %xmm0,0x40(%rbx)
    42a1:	mov    %rax,0x50(%rbx)
    42a5:	add    $0x90,%rsp
    42ac:	pop    %rbx
    42ad:	vzeroupper
    42b0:	ret
    42b1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2160:	jmp    *0x7f32(%rip)        # a098 <_mlir_ciface_main_graph_model@@Base+0x5e38>
    2166:	push   $0x13
    216b:	jmp    2020 <_init+0x20>
## main_graph_model
    23b0:	push   %r15
    23b2:	push   %r14
    23b4:	push   %r12
    23b6:	push   %rbx
    23b7:	push   %rax
    23b8:	mov    %rdx,%r14
    23bb:	mov    %rdi,%rbx
    23be:	mov    $0x48010,%edi
    23c3:	call   21e0 <malloc@plt>
    23c8:	mov    %rax,%rcx
    23cb:	add    $0xf,%rax
    23cf:	and    $0xfffffffffffffff0,%rax
    23d3:	lea    0x47500(%r14),%rdx
    23da:	cmp    %rax,%rdx
    23dd:	seta   %dil
    23e1:	lea    0x6000(%rax),%rdx
    23e8:	cmp    %rdx,%r14
    23eb:	setb   %sil
    23ef:	and    %dil,%sil
    23f2:	xor    %edi,%edi
    23f4:	vmovaps 0x4c04(%rip),%ymm0        # 7000 <_fini+0x19c>
    23fc:	vmovaps 0x4c1c(%rip),%ymm1        # 7020 <_fini+0x1bc>
    2404:	vmovaps 0x4c34(%rip),%ymm2        # 7040 <_fini+0x1dc>
    240c:	vmovaps 0x4c4c(%rip),%ymm3        # 7060 <_fini+0x1fc>
    2414:	vmovaps 0x4c64(%rip),%ymm4        # 7080 <_fini+0x21c>
    241c:	vmovaps 0x4c7c(%rip),%ymm5        # 70a0 <_fini+0x23c>
    2424:	vmovaps 0x4c94(%rip),%ymm6        # 70c0 <_fini+0x25c>
    242c:	vmovaps 0x4cac(%rip),%ymm7        # 70e0 <_fini+0x27c>
    2434:	vmovaps 0x4cc4(%rip),%ymm8        # 7100 <_fini+0x29c>
    243c:	vmovaps 0x4cdc(%rip),%ymm9        # 7120 <_fini+0x2bc>
    2444:	vmovaps 0x4cf4(%rip),%ymm10        # 7140 <_fini+0x2dc>
    244c:	vmovaps 0x4d0c(%rip),%ymm11        # 7160 <_fini+0x2fc>
    2454:	mov    %rax,%r8
    2457:	mov    %r14,%r9
    245a:	jmp    25b2 <main_graph_model+0x202>
    245f:	nop
    2460:	lea    (%r14,%rdi,4),%r10
    2464:	kxnorb %k0,%k0,%k1
    2468:	vxorps %xmm12,%xmm12,%xmm12
    246d:	vgatherdps (%r10,%ymm0,1),%ymm12{%k1}
    2474:	kxnorb %k0,%k0,%k1
    2478:	vxorps %xmm13,%xmm13,%xmm13
    247d:	vgatherdps (%r10,%ymm1,1),%ymm13{%k1}
    2484:	kxnorb %k0,%k0,%k1
    2488:	vxorps %xmm14,%xmm14,%xmm14
    248d:	vgatherdps (%r10,%ymm2,1),%ymm14{%k1}
    2494:	mov    %rdi,%r11
    2497:	kxnorb %k0,%k0,%k1
    249b:	vxorps %xmm15,%xmm15,%xmm15
    24a0:	vgatherdps (%r10,%ymm3,1),%ymm15{%k1}
    24a7:	shl    $0x7,%r11
    24ab:	lea    (%r11,%r11,2),%r11
    24af:	vmovups %ymm12,(%rax,%r11,1)
    24b5:	vmovups %ymm13,0x20(%rax,%r11,1)
    24bc:	vmovups %ymm14,0x40(%rax,%r11,1)
    24c3:	vmovups %ymm15,0x60(%rax,%r11,1)
    24ca:	kxnorb %k0,%k0,%k1
    24ce:	vxorps %xmm12,%xmm12,%xmm12
    24d3:	vgatherdps (%r10,%ymm4,1),%ymm12{%k1}
    24da:	kxnorb %k0,%k0,%k1
    24de:	vxorps %xmm13,%xmm13,%xmm13
    24e3:	vgatherdps (%r10,%ymm5,1),%ymm13{%k1}
    24ea:	kxnorb %k0,%k0,%k1
    24ee:	vxorps %xmm14,%xmm14,%xmm14
    24f3:	vgatherdps (%r10,%ymm6,1),%ymm14{%k1}
    24fa:	kxnorb %k0,%k0,%k1
    24fe:	vxorps %xmm15,%xmm15,%xmm15
    2503:	vgatherdps (%r10,%ymm7,1),%ymm15{%k1}
    250a:	vmovups %ymm12,0x80(%rax,%r11,1)
    2514:	vmovups %ymm13,0xa0(%rax,%r11,1)
    251e:	vmovups %ymm14,0xc0(%rax,%r11,1)
    2528:	vmovups %ymm15,0xe0(%rax,%r11,1)
    2532:	kxnorb %k0,%k0,%k1
    2536:	vxorps %xmm12,%xmm12,%xmm12
    253b:	vgatherdps (%r10,%ymm8,1),%ymm12{%k1}
    2542:	kxnorb %k0,%k0,%k1
    2546:	vxorps %xmm13,%xmm13,%xmm13
    254b:	vgatherdps (%r10,%ymm9,1),%ymm13{%k1}
    2552:	kxnorb %k0,%k0,%k1
    2556:	vxorps %xmm14,%xmm14,%xmm14
    255b:	vgatherdps (%r10,%ymm10,1),%ymm14{%k1}
    2562:	kxnorb %k0,%k0,%k1
    2566:	vxorps %xmm15,%xmm15,%xmm15
    256b:	vgatherdps (%r10,%ymm11,1),%ymm15{%k1}
    2572:	vmovups %ymm12,0x100(%rax,%r11,1)
    257c:	vmovups %ymm13,0x120(%rax,%r11,1)
    2586:	vmovups %ymm14,0x140(%rax,%r11,1)
    2590:	vmovups %ymm15,0x160(%rax,%r11,1)
    259a:	inc    %rdi
    259d:	add    $0x4,%r9
    25a1:	add    $0x180,%r8
    25a8:	cmp    $0x40,%rdi
    25ac:	je     26a5 <main_graph_model+0x2f5>
    25b2:	test   %sil,%sil
    25b5:	je     2460 <main_graph_model+0xb0>
    25bb:	mov    $0xb,%r10d
    25c1:	mov    %r9,%r11
    25c4:	data16 data16 cs nopw 0x0(%rax,%rax,1)
    25d0:	vmovss (%r11),%xmm12
    25d5:	vmovss %xmm12,-0x2c(%r8,%r10,4)
    25dc:	vmovss 0xc00(%r11),%xmm12
    25e5:	vmovss %xmm12,-0x28(%r8,%r10,4)
    25ec:	vmovss 0x1800(%r11),%xmm12
    25f5:	vmovss %xmm12,-0x24(%r8,%r10,4)
    25fc:	vmovss 0x2400(%r11),%xmm12
    2605:	vmovss %xmm12,-0x20(%r8,%r10,4)
    260c:	vmovss 0x3000(%r11),%xmm12
    2615:	vmovss %xmm12,-0x1c(%r8,%r10,4)
    261c:	vmovss 0x3c00(%r11),%xmm12
    2625:	vmovss %xmm12,-0x18(%r8,%r10,4)
    262c:	vmovss 0x4800(%r11),%xmm12
    2635:	vmovss %xmm12,-0x14(%r8,%r10,4)
    263c:	vmovss 0x5400(%r11),%xmm12
    2645:	vmovss %xmm12,-0x10(%r8,%r10,4)
    264c:	vmovss 0x6000(%r11),%xmm12
    2655:	vmovss %xmm12,-0xc(%r8,%r10,4)
    265c:	vmovss 0x6c00(%r11),%xmm12
    2665:	vmovss %xmm12,-0x8(%r8,%r10,4)
    266c:	vmovss 0x7800(%r11),%xmm12
    2675:	vmovss %xmm12,-0x4(%r8,%r10,4)
    267c:	vmovss 0x8400(%r11),%xmm12
    2685:	vmovss %xmm12,(%r8,%r10,4)
    268b:	add    $0x9000,%r11
    2692:	add    $0xc,%r10
    2696:	cmp    $0x6b,%r10
    269a:	jne    25d0 <main_graph_model+0x220>
    26a0:	jmp    259a <main_graph_model+0x1ea>
    26a5:	lea    0x100(%r14),%rdi
    26ac:	lea    0xc000(%rax),%rsi
    26b3:	lea    0x47600(%r14),%r8
    26ba:	cmp    %r8,%rdx
    26bd:	setb   %r9b
    26c1:	cmp    %rsi,%rdi
    26c4:	setb   %r8b
    26c8:	and    %r9b,%r8b
    26cb:	lea    0x602c(%rax),%r9
    26d2:	xor    %r10d,%r10d
    26d5:	mov    %rdi,%r11
    26d8:	jmp    2832 <main_graph_model+0x482>
    26dd:	nopl   (%rax)
    26e0:	lea    (%rdi,%r10,4),%r15
    26e4:	vxorps %xmm12,%xmm12,%xmm12
    26e9:	kxnorb %k0,%k0,%k1
    26ed:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    26f4:	vxorps %xmm13,%xmm13,%xmm13
    26f9:	kxnorb %k0,%k0,%k1
    26fd:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    2704:	vxorps %xmm14,%xmm14,%xmm14
    2709:	kxnorb %k0,%k0,%k1
    270d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    2714:	mov    %r10,%r12
    2717:	vxorps %xmm15,%xmm15,%xmm15
    271c:	kxnorb %k0,%k0,%k1
    2720:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    2727:	shl    $0x7,%r12
    272b:	lea    (%r12,%r12,2),%r12
    272f:	vmovups %ymm12,(%rdx,%r12,1)
    2735:	vmovups %ymm13,0x20(%rdx,%r12,1)
    273c:	vmovups %ymm14,0x40(%rdx,%r12,1)
    2743:	vmovups %ymm15,0x60(%rdx,%r12,1)
    274a:	vxorps %xmm12,%xmm12,%xmm12
    274f:	kxnorb %k0,%k0,%k1
    2753:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    275a:	vxorps %xmm13,%xmm13,%xmm13
    275f:	kxnorb %k0,%k0,%k1
    2763:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    276a:	vxorps %xmm14,%xmm14,%xmm14
    276f:	kxnorb %k0,%k0,%k1
    2773:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    277a:	vxorps %xmm15,%xmm15,%xmm15
    277f:	kxnorb %k0,%k0,%k1
    2783:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    278a:	vmovups %ymm12,0x80(%rdx,%r12,1)
    2794:	vmovups %ymm13,0xa0(%rdx,%r12,1)
    279e:	vmovups %ymm14,0xc0(%rdx,%r12,1)
    27a8:	vmovups %ymm15,0xe0(%rdx,%r12,1)
    27b2:	vxorps %xmm12,%xmm12,%xmm12
    27b7:	kxnorb %k0,%k0,%k1
    27bb:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    27c2:	vxorps %xmm13,%xmm13,%xmm13
    27c7:	kxnorb %k0,%k0,%k1
    27cb:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    27d2:	vxorps %xmm14,%xmm14,%xmm14
    27d7:	kxnorb %k0,%k0,%k1
    27db:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    27e2:	vxorps %xmm15,%xmm15,%xmm15
    27e7:	kxnorb %k0,%k0,%k1
    27eb:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    27f2:	vmovups %ymm12,0x100(%rdx,%r12,1)
    27fc:	vmovups %ymm13,0x120(%rdx,%r12,1)
    2806:	vmovups %ymm14,0x140(%rdx,%r12,1)
    2810:	vmovups %ymm15,0x160(%rdx,%r12,1)
    281a:	inc    %r10
    281d:	add    $0x4,%r11
    2821:	add    $0x180,%r9
    2828:	cmp    $0x40,%r10
    282c:	je     2925 <main_graph_model+0x575>
    2832:	test   %r8b,%r8b
    2835:	je     26e0 <main_graph_model+0x330>
    283b:	mov    %r11,%r15
    283e:	xor    %r12d,%r12d
    2841:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2850:	vmovss (%r15),%xmm12
    2855:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    285c:	vmovss 0xc00(%r15),%xmm12
    2865:	vmovss %xmm12,-0x28(%r9,%r12,4)
    286c:	vmovss 0x1800(%r15),%xmm12
    2875:	vmovss %xmm12,-0x24(%r9,%r12,4)
    287c:	vmovss 0x2400(%r15),%xmm12
    2885:	vmovss %xmm12,-0x20(%r9,%r12,4)
    288c:	vmovss 0x3000(%r15),%xmm12
    2895:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    289c:	vmovss 0x3c00(%r15),%xmm12
    28a5:	vmovss %xmm12,-0x18(%r9,%r12,4)
    28ac:	vmovss 0x4800(%r15),%xmm12
    28b5:	vmovss %xmm12,-0x14(%r9,%r12,4)
    28bc:	vmovss 0x5400(%r15),%xmm12
    28c5:	vmovss %xmm12,-0x10(%r9,%r12,4)
    28cc:	vmovss 0x6000(%r15),%xmm12
    28d5:	vmovss %xmm12,-0xc(%r9,%r12,4)
    28dc:	vmovss 0x6c00(%r15),%xmm12
    28e5:	vmovss %xmm12,-0x8(%r9,%r12,4)
    28ec:	vmovss 0x7800(%r15),%xmm12
    28f5:	vmovss %xmm12,-0x4(%r9,%r12,4)
    28fc:	vmovss 0x8400(%r15),%xmm12
    2905:	vmovss %xmm12,(%r9,%r12,4)
    290b:	add    $0xc,%r12
    290f:	add    $0x9000,%r15
    2916:	cmp    $0x60,%r12
    291a:	jne    2850 <main_graph_model+0x4a0>
    2920:	jmp    281a <main_graph_model+0x46a>
    2925:	lea    0x200(%r14),%rdi
    292c:	lea    0x12000(%rax),%rdx
    2933:	lea    0x47700(%r14),%r8
    293a:	cmp    %r8,%rsi
    293d:	setb   %r9b
    2941:	cmp    %rdx,%rdi
    2944:	setb   %r8b
    2948:	and    %r9b,%r8b
    294b:	lea    0xc02c(%rax),%r9
    2952:	xor    %r10d,%r10d
    2955:	mov    %rdi,%r11
    2958:	jmp    2ab2 <main_graph_model+0x702>
    295d:	nopl   (%rax)
    2960:	lea    (%rdi,%r10,4),%r15
    2964:	kxnorb %k0,%k0,%k1
    2968:	vxorps %xmm12,%xmm12,%xmm12
    296d:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    2974:	kxnorb %k0,%k0,%k1
    2978:	vxorps %xmm13,%xmm13,%xmm13
    297d:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    2984:	kxnorb %k0,%k0,%k1
    2988:	vxorps %xmm14,%xmm14,%xmm14
    298d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    2994:	mov    %r10,%r12
    2997:	kxnorb %k0,%k0,%k1
    299b:	vxorps %xmm15,%xmm15,%xmm15
    29a0:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    29a7:	shl    $0x7,%r12
    29ab:	lea    (%r12,%r12,2),%r12
    29af:	vmovups %ymm12,(%rsi,%r12,1)
    29b5:	vmovups %ymm13,0x20(%rsi,%r12,1)
    29bc:	vmovups %ymm14,0x40(%rsi,%r12,1)
    29c3:	vmovups %ymm15,0x60(%rsi,%r12,1)
    29ca:	kxnorb %k0,%k0,%k1
    29ce:	vxorps %xmm12,%xmm12,%xmm12
    29d3:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    29da:	kxnorb %k0,%k0,%k1
    29de:	vxorps %xmm13,%xmm13,%xmm13
    29e3:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    29ea:	kxnorb %k0,%k0,%k1
    29ee:	vxorps %xmm14,%xmm14,%xmm14
    29f3:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    29fa:	kxnorb %k0,%k0,%k1
    29fe:	vxorps %xmm15,%xmm15,%xmm15
    2a03:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    2a0a:	vmovups %ymm12,0x80(%rsi,%r12,1)
    2a14:	vmovups %ymm13,0xa0(%rsi,%r12,1)
    2a1e:	vmovups %ymm14,0xc0(%rsi,%r12,1)
    2a28:	vmovups %ymm15,0xe0(%rsi,%r12,1)
    2a32:	kxnorb %k0,%k0,%k1
    2a36:	vxorps %xmm12,%xmm12,%xmm12
    2a3b:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    2a42:	kxnorb %k0,%k0,%k1
    2a46:	vxorps %xmm13,%xmm13,%xmm13
    2a4b:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    2a52:	kxnorb %k0,%k0,%k1
    2a56:	vxorps %xmm14,%xmm14,%xmm14
    2a5b:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    2a62:	kxnorb %k0,%k0,%k1
    2a66:	vxorps %xmm15,%xmm15,%xmm15
    2a6b:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    2a72:	vmovups %ymm12,0x100(%rsi,%r12,1)
    2a7c:	vmovups %ymm13,0x120(%rsi,%r12,1)
    2a86:	vmovups %ymm14,0x140(%rsi,%r12,1)
    2a90:	vmovups %ymm15,0x160(%rsi,%r12,1)
    2a9a:	inc    %r10
    2a9d:	add    $0x4,%r11
    2aa1:	add    $0x180,%r9
    2aa8:	cmp    $0x40,%r10
    2aac:	je     2ba5 <main_graph_model+0x7f5>
    2ab2:	test   %r8b,%r8b
    2ab5:	je     2960 <main_graph_model+0x5b0>
    2abb:	mov    %r11,%r15
    2abe:	xor    %r12d,%r12d
    2ac1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2ad0:	vmovss (%r15),%xmm12
    2ad5:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    2adc:	vmovss 0xc00(%r15),%xmm12
    2ae5:	vmovss %xmm12,-0x28(%r9,%r12,4)
    2aec:	vmovss 0x1800(%r15),%xmm12
    2af5:	vmovss %xmm12,-0x24(%r9,%r12,4)
    2afc:	vmovss 0x2400(%r15),%xmm12
    2b05:	vmovss %xmm12,-0x20(%r9,%r12,4)
    2b0c:	vmovss 0x3000(%r15),%xmm12
    2b15:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    2b1c:	vmovss 0x3c00(%r15),%xmm12
    2b25:	vmovss %xmm12,-0x18(%r9,%r12,4)
    2b2c:	vmovss 0x4800(%r15),%xmm12
    2b35:	vmovss %xmm12,-0x14(%r9,%r12,4)
    2b3c:	vmovss 0x5400(%r15),%xmm12
    2b45:	vmovss %xmm12,-0x10(%r9,%r12,4)
    2b4c:	vmovss 0x6000(%r15),%xmm12
    2b55:	vmovss %xmm12,-0xc(%r9,%r12,4)
    2b5c:	vmovss 0x6c00(%r15),%xmm12
    2b65:	vmovss %xmm12,-0x8(%r9,%r12,4)
    2b6c:	vmovss 0x7800(%r15),%xmm12
    2b75:	vmovss %xmm12,-0x4(%r9,%r12,4)
    2b7c:	vmovss 0x8400(%r15),%xmm12
    2b85:	vmovss %xmm12,(%r9,%r12,4)
    2b8b:	add    $0xc,%r12
    2b8f:	add    $0x9000,%r15
    2b96:	cmp    $0x60,%r12
    2b9a:	jne    2ad0 <main_graph_model+0x720>
    2ba0:	jmp    2a9a <main_graph_model+0x6ea>
    2ba5:	lea    0x300(%r14),%rdi
    2bac:	lea    0x18000(%rax),%rsi
    2bb3:	lea    0x47800(%r14),%r8
    2bba:	cmp    %r8,%rdx
    2bbd:	setb   %r9b
    2bc1:	cmp    %rsi,%rdi
    2bc4:	setb   %r8b
    2bc8:	and    %r9b,%r8b
    2bcb:	lea    0x1202c(%rax),%r9
    2bd2:	xor    %r10d,%r10d
    2bd5:	mov    %rdi,%r11
    2bd8:	jmp    2d32 <main_graph_model+0x982>
    2bdd:	nopl   (%rax)
    2be0:	lea    (%rdi,%r10,4),%r15
    2be4:	kxnorb %k0,%k0,%k1
    2be8:	vxorps %xmm12,%xmm12,%xmm12
    2bed:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    2bf4:	kxnorb %k0,%k0,%k1
    2bf8:	vxorps %xmm13,%xmm13,%xmm13
    2bfd:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    2c04:	kxnorb %k0,%k0,%k1
    2c08:	vxorps %xmm14,%xmm14,%xmm14
    2c0d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    2c14:	mov    %r10,%r12
    2c17:	kxnorb %k0,%k0,%k1
    2c1b:	vxorps %xmm15,%xmm15,%xmm15
    2c20:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    2c27:	shl    $0x7,%r12
    2c2b:	lea    (%r12,%r12,2),%r12
    2c2f:	vmovups %ymm12,(%rdx,%r12,1)
    2c35:	vmovups %ymm13,0x20(%rdx,%r12,1)
    2c3c:	vmovups %ymm14,0x40(%rdx,%r12,1)
    2c43:	vmovups %ymm15,0x60(%rdx,%r12,1)
    2c4a:	kxnorb %k0,%k0,%k1
    2c4e:	vxorps %xmm12,%xmm12,%xmm12
    2c53:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    2c5a:	kxnorb %k0,%k0,%k1
    2c5e:	vxorps %xmm13,%xmm13,%xmm13
    2c63:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    2c6a:	kxnorb %k0,%k0,%k1
    2c6e:	vxorps %xmm14,%xmm14,%xmm14
    2c73:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    2c7a:	kxnorb %k0,%k0,%k1
    2c7e:	vxorps %xmm15,%xmm15,%xmm15
    2c83:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    2c8a:	vmovups %ymm12,0x80(%rdx,%r12,1)
    2c94:	vmovups %ymm13,0xa0(%rdx,%r12,1)
    2c9e:	vmovups %ymm14,0xc0(%rdx,%r12,1)
    2ca8:	vmovups %ymm15,0xe0(%rdx,%r12,1)
    2cb2:	kxnorb %k0,%k0,%k1
    2cb6:	vxorps %xmm12,%xmm12,%xmm12
    2cbb:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    2cc2:	kxnorb %k0,%k0,%k1
    2cc6:	vxorps %xmm13,%xmm13,%xmm13
    2ccb:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    2cd2:	kxnorb %k0,%k0,%k1
    2cd6:	vxorps %xmm14,%xmm14,%xmm14
    2cdb:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    2ce2:	kxnorb %k0,%k0,%k1
    2ce6:	vxorps %xmm15,%xmm15,%xmm15
    2ceb:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    2cf2:	vmovups %ymm12,0x100(%rdx,%r12,1)
    2cfc:	vmovups %ymm13,0x120(%rdx,%r12,1)
    2d06:	vmovups %ymm14,0x140(%rdx,%r12,1)
    2d10:	vmovups %ymm15,0x160(%rdx,%r12,1)
    2d1a:	inc    %r10
    2d1d:	add    $0x4,%r11
    2d21:	add    $0x180,%r9
    2d28:	cmp    $0x40,%r10
    2d2c:	je     2e25 <main_graph_model+0xa75>
    2d32:	test   %r8b,%r8b
    2d35:	je     2be0 <main_graph_model+0x830>
    2d3b:	mov    %r11,%r15
    2d3e:	xor    %r12d,%r12d
    2d41:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2d50:	vmovss (%r15),%xmm12
    2d55:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    2d5c:	vmovss 0xc00(%r15),%xmm12
    2d65:	vmovss %xmm12,-0x28(%r9,%r12,4)
    2d6c:	vmovss 0x1800(%r15),%xmm12
    2d75:	vmovss %xmm12,-0x24(%r9,%r12,4)
    2d7c:	vmovss 0x2400(%r15),%xmm12
    2d85:	vmovss %xmm12,-0x20(%r9,%r12,4)
    2d8c:	vmovss 0x3000(%r15),%xmm12
    2d95:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    2d9c:	vmovss 0x3c00(%r15),%xmm12
    2da5:	vmovss %xmm12,-0x18(%r9,%r12,4)
    2dac:	vmovss 0x4800(%r15),%xmm12
    2db5:	vmovss %xmm12,-0x14(%r9,%r12,4)
    2dbc:	vmovss 0x5400(%r15),%xmm12
    2dc5:	vmovss %xmm12,-0x10(%r9,%r12,4)
    2dcc:	vmovss 0x6000(%r15),%xmm12
    2dd5:	vmovss %xmm12,-0xc(%r9,%r12,4)
    2ddc:	vmovss 0x6c00(%r15),%xmm12
    2de5:	vmovss %xmm12,-0x8(%r9,%r12,4)
    2dec:	vmovss 0x7800(%r15),%xmm12
    2df5:	vmovss %xmm12,-0x4(%r9,%r12,4)
    2dfc:	vmovss 0x8400(%r15),%xmm12
    2e05:	vmovss %xmm12,(%r9,%r12,4)
    2e0b:	add    $0xc,%r12
    2e0f:	add    $0x9000,%r15
    2e16:	cmp    $0x60,%r12
    2e1a:	jne    2d50 <main_graph_model+0x9a0>
    2e20:	jmp    2d1a <main_graph_model+0x96a>
    2e25:	lea    0x400(%r14),%rdi
    2e2c:	lea    0x1e000(%rax),%rdx
    2e33:	lea    0x47900(%r14),%r8
    2e3a:	cmp    %r8,%rsi
    2e3d:	setb   %r9b
    2e41:	cmp    %rdx,%rdi
    2e44:	setb   %r8b
    2e48:	and    %r9b,%r8b
    2e4b:	lea    0x1802c(%rax),%r9
    2e52:	xor    %r10d,%r10d
    2e55:	mov    %rdi,%r11
    2e58:	jmp    2fb2 <main_graph_model+0xc02>
    2e5d:	nopl   (%rax)
    2e60:	lea    (%rdi,%r10,4),%r15
    2e64:	kxnorb %k0,%k0,%k1
    2e68:	vxorps %xmm12,%xmm12,%xmm12
    2e6d:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    2e74:	kxnorb %k0,%k0,%k1
    2e78:	vxorps %xmm13,%xmm13,%xmm13
    2e7d:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    2e84:	kxnorb %k0,%k0,%k1
    2e88:	vxorps %xmm14,%xmm14,%xmm14
    2e8d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    2e94:	mov    %r10,%r12
    2e97:	kxnorb %k0,%k0,%k1
    2e9b:	vxorps %xmm15,%xmm15,%xmm15
    2ea0:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    2ea7:	shl    $0x7,%r12
    2eab:	lea    (%r12,%r12,2),%r12
    2eaf:	vmovups %ymm12,(%rsi,%r12,1)
    2eb5:	vmovups %ymm13,0x20(%rsi,%r12,1)
    2ebc:	vmovups %ymm14,0x40(%rsi,%r12,1)
    2ec3:	vmovups %ymm15,0x60(%rsi,%r12,1)
    2eca:	kxnorb %k0,%k0,%k1
    2ece:	vxorps %xmm12,%xmm12,%xmm12
    2ed3:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    2eda:	kxnorb %k0,%k0,%k1
    2ede:	vxorps %xmm13,%xmm13,%xmm13
    2ee3:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    2eea:	kxnorb %k0,%k0,%k1
    2eee:	vxorps %xmm14,%xmm14,%xmm14
    2ef3:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    2efa:	kxnorb %k0,%k0,%k1
    2efe:	vxorps %xmm15,%xmm15,%xmm15
    2f03:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    2f0a:	vmovups %ymm12,0x80(%rsi,%r12,1)
    2f14:	vmovups %ymm13,0xa0(%rsi,%r12,1)
    2f1e:	vmovups %ymm14,0xc0(%rsi,%r12,1)
    2f28:	vmovups %ymm15,0xe0(%rsi,%r12,1)
    2f32:	kxnorb %k0,%k0,%k1
    2f36:	vxorps %xmm12,%xmm12,%xmm12
    2f3b:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    2f42:	kxnorb %k0,%k0,%k1
    2f46:	vxorps %xmm13,%xmm13,%xmm13
    2f4b:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    2f52:	kxnorb %k0,%k0,%k1
    2f56:	vxorps %xmm14,%xmm14,%xmm14
    2f5b:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    2f62:	kxnorb %k0,%k0,%k1
    2f66:	vxorps %xmm15,%xmm15,%xmm15
    2f6b:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    2f72:	vmovups %ymm12,0x100(%rsi,%r12,1)
    2f7c:	vmovups %ymm13,0x120(%rsi,%r12,1)
    2f86:	vmovups %ymm14,0x140(%rsi,%r12,1)
    2f90:	vmovups %ymm15,0x160(%rsi,%r12,1)
    2f9a:	inc    %r10
    2f9d:	add    $0x4,%r11
    2fa1:	add    $0x180,%r9
    2fa8:	cmp    $0x40,%r10
    2fac:	je     30a5 <main_graph_model+0xcf5>
    2fb2:	test   %r8b,%r8b
    2fb5:	je     2e60 <main_graph_model+0xab0>
    2fbb:	mov    %r11,%r15
    2fbe:	xor    %r12d,%r12d
    2fc1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2fd0:	vmovss (%r15),%xmm12
    2fd5:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    2fdc:	vmovss 0xc00(%r15),%xmm12
    2fe5:	vmovss %xmm12,-0x28(%r9,%r12,4)
    2fec:	vmovss 0x1800(%r15),%xmm12
    2ff5:	vmovss %xmm12,-0x24(%r9,%r12,4)
    2ffc:	vmovss 0x2400(%r15),%xmm12
    3005:	vmovss %xmm12,-0x20(%r9,%r12,4)
    300c:	vmovss 0x3000(%r15),%xmm12
    3015:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    301c:	vmovss 0x3c00(%r15),%xmm12
    3025:	vmovss %xmm12,-0x18(%r9,%r12,4)
    302c:	vmovss 0x4800(%r15),%xmm12
    3035:	vmovss %xmm12,-0x14(%r9,%r12,4)
    303c:	vmovss 0x5400(%r15),%xmm12
    3045:	vmovss %xmm12,-0x10(%r9,%r12,4)
    304c:	vmovss 0x6000(%r15),%xmm12
    3055:	vmovss %xmm12,-0xc(%r9,%r12,4)
    305c:	vmovss 0x6c00(%r15),%xmm12
    3065:	vmovss %xmm12,-0x8(%r9,%r12,4)
    306c:	vmovss 0x7800(%r15),%xmm12
    3075:	vmovss %xmm12,-0x4(%r9,%r12,4)
    307c:	vmovss 0x8400(%r15),%xmm12
    3085:	vmovss %xmm12,(%r9,%r12,4)
    308b:	add    $0xc,%r12
    308f:	add    $0x9000,%r15
    3096:	cmp    $0x60,%r12
    309a:	jne    2fd0 <main_graph_model+0xc20>
    30a0:	jmp    2f9a <main_graph_model+0xbea>
    30a5:	lea    0x500(%r14),%rdi
    30ac:	lea    0x24000(%rax),%rsi
    30b3:	lea    0x47a00(%r14),%r8
    30ba:	cmp    %r8,%rdx
    30bd:	setb   %r9b
    30c1:	cmp    %rsi,%rdi
    30c4:	setb   %r8b
    30c8:	and    %r9b,%r8b
    30cb:	lea    0x1e02c(%rax),%r9
    30d2:	xor    %r10d,%r10d
    30d5:	mov    %rdi,%r11
    30d8:	jmp    3232 <main_graph_model+0xe82>
    30dd:	nopl   (%rax)
    30e0:	lea    (%rdi,%r10,4),%r15
    30e4:	vxorps %xmm12,%xmm12,%xmm12
    30e9:	kxnorb %k0,%k0,%k1
    30ed:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    30f4:	vxorps %xmm13,%xmm13,%xmm13
    30f9:	kxnorb %k0,%k0,%k1
    30fd:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3104:	vxorps %xmm14,%xmm14,%xmm14
    3109:	kxnorb %k0,%k0,%k1
    310d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3114:	mov    %r10,%r12
    3117:	vxorps %xmm15,%xmm15,%xmm15
    311c:	kxnorb %k0,%k0,%k1
    3120:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    3127:	shl    $0x7,%r12
    312b:	lea    (%r12,%r12,2),%r12
    312f:	vmovups %ymm12,(%rdx,%r12,1)
    3135:	vmovups %ymm13,0x20(%rdx,%r12,1)
    313c:	vmovups %ymm14,0x40(%rdx,%r12,1)
    3143:	vmovups %ymm15,0x60(%rdx,%r12,1)
    314a:	vxorps %xmm12,%xmm12,%xmm12
    314f:	kxnorb %k0,%k0,%k1
    3153:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    315a:	vxorps %xmm13,%xmm13,%xmm13
    315f:	kxnorb %k0,%k0,%k1
    3163:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    316a:	vxorps %xmm14,%xmm14,%xmm14
    316f:	kxnorb %k0,%k0,%k1
    3173:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    317a:	vxorps %xmm15,%xmm15,%xmm15
    317f:	kxnorb %k0,%k0,%k1
    3183:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    318a:	vmovups %ymm12,0x80(%rdx,%r12,1)
    3194:	vmovups %ymm13,0xa0(%rdx,%r12,1)
    319e:	vmovups %ymm14,0xc0(%rdx,%r12,1)
    31a8:	vmovups %ymm15,0xe0(%rdx,%r12,1)
    31b2:	vxorps %xmm12,%xmm12,%xmm12
    31b7:	kxnorb %k0,%k0,%k1
    31bb:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    31c2:	vxorps %xmm13,%xmm13,%xmm13
    31c7:	kxnorb %k0,%k0,%k1
    31cb:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    31d2:	vxorps %xmm14,%xmm14,%xmm14
    31d7:	kxnorb %k0,%k0,%k1
    31db:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    31e2:	vxorps %xmm15,%xmm15,%xmm15
    31e7:	kxnorb %k0,%k0,%k1
    31eb:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    31f2:	vmovups %ymm12,0x100(%rdx,%r12,1)
    31fc:	vmovups %ymm13,0x120(%rdx,%r12,1)
    3206:	vmovups %ymm14,0x140(%rdx,%r12,1)
    3210:	vmovups %ymm15,0x160(%rdx,%r12,1)
    321a:	inc    %r10
    321d:	add    $0x4,%r11
    3221:	add    $0x180,%r9
    3228:	cmp    $0x40,%r10
    322c:	je     3325 <main_graph_model+0xf75>
    3232:	test   %r8b,%r8b
    3235:	je     30e0 <main_graph_model+0xd30>
    323b:	mov    %r11,%r15
    323e:	xor    %r12d,%r12d
    3241:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    3250:	vmovss (%r15),%xmm12
    3255:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    325c:	vmovss 0xc00(%r15),%xmm12
    3265:	vmovss %xmm12,-0x28(%r9,%r12,4)
    326c:	vmovss 0x1800(%r15),%xmm12
    3275:	vmovss %xmm12,-0x24(%r9,%r12,4)
    327c:	vmovss 0x2400(%r15),%xmm12
    3285:	vmovss %xmm12,-0x20(%r9,%r12,4)
    328c:	vmovss 0x3000(%r15),%xmm12
    3295:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    329c:	vmovss 0x3c00(%r15),%xmm12
    32a5:	vmovss %xmm12,-0x18(%r9,%r12,4)
    32ac:	vmovss 0x4800(%r15),%xmm12
    32b5:	vmovss %xmm12,-0x14(%r9,%r12,4)
    32bc:	vmovss 0x5400(%r15),%xmm12
    32c5:	vmovss %xmm12,-0x10(%r9,%r12,4)
    32cc:	vmovss 0x6000(%r15),%xmm12
    32d5:	vmovss %xmm12,-0xc(%r9,%r12,4)
    32dc:	vmovss 0x6c00(%r15),%xmm12
    32e5:	vmovss %xmm12,-0x8(%r9,%r12,4)
    32ec:	vmovss 0x7800(%r15),%xmm12
    32f5:	vmovss %xmm12,-0x4(%r9,%r12,4)
    32fc:	vmovss 0x8400(%r15),%xmm12
    3305:	vmovss %xmm12,(%r9,%r12,4)
    330b:	add    $0xc,%r12
    330f:	add    $0x9000,%r15
    3316:	cmp    $0x60,%r12
    331a:	jne    3250 <main_graph_model+0xea0>
    3320:	jmp    321a <main_graph_model+0xe6a>
    3325:	lea    0x600(%r14),%rdi
    332c:	lea    0x2a000(%rax),%rdx
    3333:	lea    0x47b00(%r14),%r8
    333a:	cmp    %r8,%rsi
    333d:	setb   %r9b
    3341:	cmp    %rdx,%rdi
    3344:	setb   %r8b
    3348:	and    %r9b,%r8b
    334b:	lea    0x2402c(%rax),%r9
    3352:	xor    %r10d,%r10d
    3355:	mov    %rdi,%r11
    3358:	jmp    34b2 <main_graph_model+0x1102>
    335d:	nopl   (%rax)
    3360:	lea    (%rdi,%r10,4),%r15
    3364:	kxnorb %k0,%k0,%k1
    3368:	vxorps %xmm12,%xmm12,%xmm12
    336d:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    3374:	kxnorb %k0,%k0,%k1
    3378:	vxorps %xmm13,%xmm13,%xmm13
    337d:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3384:	kxnorb %k0,%k0,%k1
    3388:	vxorps %xmm14,%xmm14,%xmm14
    338d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3394:	mov    %r10,%r12
    3397:	kxnorb %k0,%k0,%k1
    339b:	vxorps %xmm15,%xmm15,%xmm15
    33a0:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    33a7:	shl    $0x7,%r12
    33ab:	lea    (%r12,%r12,2),%r12
    33af:	vmovups %ymm12,(%rsi,%r12,1)
    33b5:	vmovups %ymm13,0x20(%rsi,%r12,1)
    33bc:	vmovups %ymm14,0x40(%rsi,%r12,1)
    33c3:	vmovups %ymm15,0x60(%rsi,%r12,1)
    33ca:	kxnorb %k0,%k0,%k1
    33ce:	vxorps %xmm12,%xmm12,%xmm12
    33d3:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    33da:	kxnorb %k0,%k0,%k1
    33de:	vxorps %xmm13,%xmm13,%xmm13
    33e3:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    33ea:	kxnorb %k0,%k0,%k1
    33ee:	vxorps %xmm14,%xmm14,%xmm14
    33f3:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    33fa:	kxnorb %k0,%k0,%k1
    33fe:	vxorps %xmm15,%xmm15,%xmm15
    3403:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    340a:	vmovups %ymm12,0x80(%rsi,%r12,1)
    3414:	vmovups %ymm13,0xa0(%rsi,%r12,1)
    341e:	vmovups %ymm14,0xc0(%rsi,%r12,1)
    3428:	vmovups %ymm15,0xe0(%rsi,%r12,1)
    3432:	kxnorb %k0,%k0,%k1
    3436:	vxorps %xmm12,%xmm12,%xmm12
    343b:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    3442:	kxnorb %k0,%k0,%k1
    3446:	vxorps %xmm13,%xmm13,%xmm13
    344b:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    3452:	kxnorb %k0,%k0,%k1
    3456:	vxorps %xmm14,%xmm14,%xmm14
    345b:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    3462:	kxnorb %k0,%k0,%k1
    3466:	vxorps %xmm15,%xmm15,%xmm15
    346b:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    3472:	vmovups %ymm12,0x100(%rsi,%r12,1)
    347c:	vmovups %ymm13,0x120(%rsi,%r12,1)
    3486:	vmovups %ymm14,0x140(%rsi,%r12,1)
    3490:	vmovups %ymm15,0x160(%rsi,%r12,1)
    349a:	inc    %r10
    349d:	add    $0x4,%r11
    34a1:	add    $0x180,%r9
    34a8:	cmp    $0x40,%r10
    34ac:	je     35a5 <main_graph_model+0x11f5>
    34b2:	test   %r8b,%r8b
    34b5:	je     3360 <main_graph_model+0xfb0>
    34bb:	mov    %r11,%r15
    34be:	xor    %r12d,%r12d
    34c1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    34d0:	vmovss (%r15),%xmm12
    34d5:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    34dc:	vmovss 0xc00(%r15),%xmm12
    34e5:	vmovss %xmm12,-0x28(%r9,%r12,4)
    34ec:	vmovss 0x1800(%r15),%xmm12
    34f5:	vmovss %xmm12,-0x24(%r9,%r12,4)
    34fc:	vmovss 0x2400(%r15),%xmm12
    3505:	vmovss %xmm12,-0x20(%r9,%r12,4)
    350c:	vmovss 0x3000(%r15),%xmm12
    3515:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    351c:	vmovss 0x3c00(%r15),%xmm12
    3525:	vmovss %xmm12,-0x18(%r9,%r12,4)
    352c:	vmovss 0x4800(%r15),%xmm12
    3535:	vmovss %xmm12,-0x14(%r9,%r12,4)
    353c:	vmovss 0x5400(%r15),%xmm12
    3545:	vmovss %xmm12,-0x10(%r9,%r12,4)
    354c:	vmovss 0x6000(%r15),%xmm12
    3555:	vmovss %xmm12,-0xc(%r9,%r12,4)
    355c:	vmovss 0x6c00(%r15),%xmm12
    3565:	vmovss %xmm12,-0x8(%r9,%r12,4)
    356c:	vmovss 0x7800(%r15),%xmm12
    3575:	vmovss %xmm12,-0x4(%r9,%r12,4)
    357c:	vmovss 0x8400(%r15),%xmm12
    3585:	vmovss %xmm12,(%r9,%r12,4)
    358b:	add    $0xc,%r12
    358f:	add    $0x9000,%r15
    3596:	cmp    $0x60,%r12
    359a:	jne    34d0 <main_graph_model+0x1120>
    35a0:	jmp    349a <main_graph_model+0x10ea>
    35a5:	lea    0x700(%r14),%rdi
    35ac:	lea    0x30000(%rax),%rsi
    35b3:	lea    0x47c00(%r14),%r8
    35ba:	cmp    %r8,%rdx
    35bd:	setb   %r9b
    35c1:	cmp    %rsi,%rdi
    35c4:	setb   %r8b
    35c8:	and    %r9b,%r8b
    35cb:	lea    0x2a02c(%rax),%r9
    35d2:	xor    %r10d,%r10d
    35d5:	mov    %rdi,%r11
    35d8:	jmp    3732 <main_graph_model+0x1382>
    35dd:	nopl   (%rax)
    35e0:	lea    (%rdi,%r10,4),%r15
    35e4:	kxnorb %k0,%k0,%k1
    35e8:	vxorps %xmm12,%xmm12,%xmm12
    35ed:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    35f4:	kxnorb %k0,%k0,%k1
    35f8:	vxorps %xmm13,%xmm13,%xmm13
    35fd:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3604:	kxnorb %k0,%k0,%k1
    3608:	vxorps %xmm14,%xmm14,%xmm14
    360d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3614:	mov    %r10,%r12
    3617:	kxnorb %k0,%k0,%k1
    361b:	vxorps %xmm15,%xmm15,%xmm15
    3620:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    3627:	shl    $0x7,%r12
    362b:	lea    (%r12,%r12,2),%r12
    362f:	vmovups %ymm12,(%rdx,%r12,1)
    3635:	vmovups %ymm13,0x20(%rdx,%r12,1)
    363c:	vmovups %ymm14,0x40(%rdx,%r12,1)
    3643:	vmovups %ymm15,0x60(%rdx,%r12,1)
    364a:	kxnorb %k0,%k0,%k1
    364e:	vxorps %xmm12,%xmm12,%xmm12
    3653:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    365a:	kxnorb %k0,%k0,%k1
    365e:	vxorps %xmm13,%xmm13,%xmm13
    3663:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    366a:	kxnorb %k0,%k0,%k1
    366e:	vxorps %xmm14,%xmm14,%xmm14
    3673:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    367a:	kxnorb %k0,%k0,%k1
    367e:	vxorps %xmm15,%xmm15,%xmm15
    3683:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    368a:	vmovups %ymm12,0x80(%rdx,%r12,1)
    3694:	vmovups %ymm13,0xa0(%rdx,%r12,1)
    369e:	vmovups %ymm14,0xc0(%rdx,%r12,1)
    36a8:	vmovups %ymm15,0xe0(%rdx,%r12,1)
    36b2:	kxnorb %k0,%k0,%k1
    36b6:	vxorps %xmm12,%xmm12,%xmm12
    36bb:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    36c2:	kxnorb %k0,%k0,%k1
    36c6:	vxorps %xmm13,%xmm13,%xmm13
    36cb:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    36d2:	kxnorb %k0,%k0,%k1
    36d6:	vxorps %xmm14,%xmm14,%xmm14
    36db:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    36e2:	kxnorb %k0,%k0,%k1
    36e6:	vxorps %xmm15,%xmm15,%xmm15
    36eb:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    36f2:	vmovups %ymm12,0x100(%rdx,%r12,1)
    36fc:	vmovups %ymm13,0x120(%rdx,%r12,1)
    3706:	vmovups %ymm14,0x140(%rdx,%r12,1)
    3710:	vmovups %ymm15,0x160(%rdx,%r12,1)
    371a:	inc    %r10
    371d:	add    $0x4,%r11
    3721:	add    $0x180,%r9
    3728:	cmp    $0x40,%r10
    372c:	je     3825 <main_graph_model+0x1475>
    3732:	test   %r8b,%r8b
    3735:	je     35e0 <main_graph_model+0x1230>
    373b:	mov    %r11,%r15
    373e:	xor    %r12d,%r12d
    3741:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    3750:	vmovss (%r15),%xmm12
    3755:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    375c:	vmovss 0xc00(%r15),%xmm12
    3765:	vmovss %xmm12,-0x28(%r9,%r12,4)
    376c:	vmovss 0x1800(%r15),%xmm12
    3775:	vmovss %xmm12,-0x24(%r9,%r12,4)
    377c:	vmovss 0x2400(%r15),%xmm12
    3785:	vmovss %xmm12,-0x20(%r9,%r12,4)
    378c:	vmovss 0x3000(%r15),%xmm12
    3795:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    379c:	vmovss 0x3c00(%r15),%xmm12
    37a5:	vmovss %xmm12,-0x18(%r9,%r12,4)
    37ac:	vmovss 0x4800(%r15),%xmm12
    37b5:	vmovss %xmm12,-0x14(%r9,%r12,4)
    37bc:	vmovss 0x5400(%r15),%xmm12
    37c5:	vmovss %xmm12,-0x10(%r9,%r12,4)
    37cc:	vmovss 0x6000(%r15),%xmm12
    37d5:	vmovss %xmm12,-0xc(%r9,%r12,4)
    37dc:	vmovss 0x6c00(%r15),%xmm12
    37e5:	vmovss %xmm12,-0x8(%r9,%r12,4)
    37ec:	vmovss 0x7800(%r15),%xmm12
    37f5:	vmovss %xmm12,-0x4(%r9,%r12,4)
    37fc:	vmovss 0x8400(%r15),%xmm12
    3805:	vmovss %xmm12,(%r9,%r12,4)
    380b:	add    $0xc,%r12
    380f:	add    $0x9000,%r15
    3816:	cmp    $0x60,%r12
    381a:	jne    3750 <main_graph_model+0x13a0>
    3820:	jmp    371a <main_graph_model+0x136a>
    3825:	lea    0x800(%r14),%rdi
    382c:	lea    0x36000(%rax),%rdx
    3833:	lea    0x47d00(%r14),%r8
    383a:	cmp    %r8,%rsi
    383d:	setb   %r9b
    3841:	cmp    %rdx,%rdi
    3844:	setb   %r8b
    3848:	and    %r9b,%r8b
    384b:	lea    0x3002c(%rax),%r9
    3852:	xor    %r10d,%r10d
    3855:	mov    %rdi,%r11
    3858:	jmp    39b2 <main_graph_model+0x1602>
    385d:	nopl   (%rax)
    3860:	lea    (%rdi,%r10,4),%r15
    3864:	kxnorb %k0,%k0,%k1
    3868:	vxorps %xmm12,%xmm12,%xmm12
    386d:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    3874:	kxnorb %k0,%k0,%k1
    3878:	vxorps %xmm13,%xmm13,%xmm13
    387d:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3884:	kxnorb %k0,%k0,%k1
    3888:	vxorps %xmm14,%xmm14,%xmm14
    388d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3894:	mov    %r10,%r12
    3897:	kxnorb %k0,%k0,%k1
    389b:	vxorps %xmm15,%xmm15,%xmm15
    38a0:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    38a7:	shl    $0x7,%r12
    38ab:	lea    (%r12,%r12,2),%r12
    38af:	vmovups %ymm12,(%rsi,%r12,1)
    38b5:	vmovups %ymm13,0x20(%rsi,%r12,1)
    38bc:	vmovups %ymm14,0x40(%rsi,%r12,1)
    38c3:	vmovups %ymm15,0x60(%rsi,%r12,1)
    38ca:	kxnorb %k0,%k0,%k1
    38ce:	vxorps %xmm12,%xmm12,%xmm12
    38d3:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    38da:	kxnorb %k0,%k0,%k1
    38de:	vxorps %xmm13,%xmm13,%xmm13
    38e3:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    38ea:	kxnorb %k0,%k0,%k1
    38ee:	vxorps %xmm14,%xmm14,%xmm14
    38f3:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    38fa:	kxnorb %k0,%k0,%k1
    38fe:	vxorps %xmm15,%xmm15,%xmm15
    3903:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    390a:	vmovups %ymm12,0x80(%rsi,%r12,1)
    3914:	vmovups %ymm13,0xa0(%rsi,%r12,1)
    391e:	vmovups %ymm14,0xc0(%rsi,%r12,1)
    3928:	vmovups %ymm15,0xe0(%rsi,%r12,1)
    3932:	kxnorb %k0,%k0,%k1
    3936:	vxorps %xmm12,%xmm12,%xmm12
    393b:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    3942:	kxnorb %k0,%k0,%k1
    3946:	vxorps %xmm13,%xmm13,%xmm13
    394b:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    3952:	kxnorb %k0,%k0,%k1
    3956:	vxorps %xmm14,%xmm14,%xmm14
    395b:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    3962:	kxnorb %k0,%k0,%k1
    3966:	vxorps %xmm15,%xmm15,%xmm15
    396b:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    3972:	vmovups %ymm12,0x100(%rsi,%r12,1)
    397c:	vmovups %ymm13,0x120(%rsi,%r12,1)
    3986:	vmovups %ymm14,0x140(%rsi,%r12,1)
    3990:	vmovups %ymm15,0x160(%rsi,%r12,1)
    399a:	inc    %r10
    399d:	add    $0x4,%r11
    39a1:	add    $0x180,%r9
    39a8:	cmp    $0x40,%r10
    39ac:	je     3aa5 <main_graph_model+0x16f5>
    39b2:	test   %r8b,%r8b
    39b5:	je     3860 <main_graph_model+0x14b0>
    39bb:	mov    %r11,%r15
    39be:	xor    %r12d,%r12d
    39c1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    39d0:	vmovss (%r15),%xmm12
    39d5:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    39dc:	vmovss 0xc00(%r15),%xmm12
    39e5:	vmovss %xmm12,-0x28(%r9,%r12,4)
    39ec:	vmovss 0x1800(%r15),%xmm12
    39f5:	vmovss %xmm12,-0x24(%r9,%r12,4)
    39fc:	vmovss 0x2400(%r15),%xmm12
    3a05:	vmovss %xmm12,-0x20(%r9,%r12,4)
    3a0c:	vmovss 0x3000(%r15),%xmm12
    3a15:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    3a1c:	vmovss 0x3c00(%r15),%xmm12
    3a25:	vmovss %xmm12,-0x18(%r9,%r12,4)
    3a2c:	vmovss 0x4800(%r15),%xmm12
    3a35:	vmovss %xmm12,-0x14(%r9,%r12,4)
    3a3c:	vmovss 0x5400(%r15),%xmm12
    3a45:	vmovss %xmm12,-0x10(%r9,%r12,4)
    3a4c:	vmovss 0x6000(%r15),%xmm12
    3a55:	vmovss %xmm12,-0xc(%r9,%r12,4)
    3a5c:	vmovss 0x6c00(%r15),%xmm12
    3a65:	vmovss %xmm12,-0x8(%r9,%r12,4)
    3a6c:	vmovss 0x7800(%r15),%xmm12
    3a75:	vmovss %xmm12,-0x4(%r9,%r12,4)
    3a7c:	vmovss 0x8400(%r15),%xmm12
    3a85:	vmovss %xmm12,(%r9,%r12,4)
    3a8b:	add    $0xc,%r12
    3a8f:	add    $0x9000,%r15
    3a96:	cmp    $0x60,%r12
    3a9a:	jne    39d0 <main_graph_model+0x1620>
    3aa0:	jmp    399a <main_graph_model+0x15ea>
    3aa5:	lea    0x900(%r14),%rdi
    3aac:	lea    0x3c000(%rax),%rsi
    3ab3:	lea    0x47e00(%r14),%r8
    3aba:	cmp    %r8,%rdx
    3abd:	setb   %r9b
    3ac1:	cmp    %rsi,%rdi
    3ac4:	setb   %r8b
    3ac8:	and    %r9b,%r8b
    3acb:	lea    0x3602c(%rax),%r9
    3ad2:	xor    %r10d,%r10d
    3ad5:	mov    %rdi,%r11
    3ad8:	jmp    3c32 <main_graph_model+0x1882>
    3add:	nopl   (%rax)
    3ae0:	lea    (%rdi,%r10,4),%r15
    3ae4:	vxorps %xmm12,%xmm12,%xmm12
    3ae9:	kxnorb %k0,%k0,%k1
    3aed:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    3af4:	vxorps %xmm13,%xmm13,%xmm13
    3af9:	kxnorb %k0,%k0,%k1
    3afd:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3b04:	vxorps %xmm14,%xmm14,%xmm14
    3b09:	kxnorb %k0,%k0,%k1
    3b0d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3b14:	mov    %r10,%r12
    3b17:	vxorps %xmm15,%xmm15,%xmm15
    3b1c:	kxnorb %k0,%k0,%k1
    3b20:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    3b27:	shl    $0x7,%r12
    3b2b:	lea    (%r12,%r12,2),%r12
    3b2f:	vmovups %ymm12,(%rdx,%r12,1)
    3b35:	vmovups %ymm13,0x20(%rdx,%r12,1)
    3b3c:	vmovups %ymm14,0x40(%rdx,%r12,1)
    3b43:	vmovups %ymm15,0x60(%rdx,%r12,1)
    3b4a:	vxorps %xmm12,%xmm12,%xmm12
    3b4f:	kxnorb %k0,%k0,%k1
    3b53:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    3b5a:	vxorps %xmm13,%xmm13,%xmm13
    3b5f:	kxnorb %k0,%k0,%k1
    3b63:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    3b6a:	vxorps %xmm14,%xmm14,%xmm14
    3b6f:	kxnorb %k0,%k0,%k1
    3b73:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    3b7a:	vxorps %xmm15,%xmm15,%xmm15
    3b7f:	kxnorb %k0,%k0,%k1
    3b83:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    3b8a:	vmovups %ymm12,0x80(%rdx,%r12,1)
    3b94:	vmovups %ymm13,0xa0(%rdx,%r12,1)
    3b9e:	vmovups %ymm14,0xc0(%rdx,%r12,1)
    3ba8:	vmovups %ymm15,0xe0(%rdx,%r12,1)
    3bb2:	vxorps %xmm12,%xmm12,%xmm12
    3bb7:	kxnorb %k0,%k0,%k1
    3bbb:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    3bc2:	vxorps %xmm13,%xmm13,%xmm13
    3bc7:	kxnorb %k0,%k0,%k1
    3bcb:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    3bd2:	vxorps %xmm14,%xmm14,%xmm14
    3bd7:	kxnorb %k0,%k0,%k1
    3bdb:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    3be2:	vxorps %xmm15,%xmm15,%xmm15
    3be7:	kxnorb %k0,%k0,%k1
    3beb:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    3bf2:	vmovups %ymm12,0x100(%rdx,%r12,1)
    3bfc:	vmovups %ymm13,0x120(%rdx,%r12,1)
    3c06:	vmovups %ymm14,0x140(%rdx,%r12,1)
    3c10:	vmovups %ymm15,0x160(%rdx,%r12,1)
    3c1a:	inc    %r10
    3c1d:	add    $0x4,%r11
    3c21:	add    $0x180,%r9
    3c28:	cmp    $0x40,%r10
    3c2c:	je     3d25 <main_graph_model+0x1975>
    3c32:	test   %r8b,%r8b
    3c35:	je     3ae0 <main_graph_model+0x1730>
    3c3b:	mov    %r11,%r15
    3c3e:	xor    %r12d,%r12d
    3c41:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    3c50:	vmovss (%r15),%xmm12
    3c55:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    3c5c:	vmovss 0xc00(%r15),%xmm12
    3c65:	vmovss %xmm12,-0x28(%r9,%r12,4)
    3c6c:	vmovss 0x1800(%r15),%xmm12
    3c75:	vmovss %xmm12,-0x24(%r9,%r12,4)
    3c7c:	vmovss 0x2400(%r15),%xmm12
    3c85:	vmovss %xmm12,-0x20(%r9,%r12,4)
    3c8c:	vmovss 0x3000(%r15),%xmm12
    3c95:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    3c9c:	vmovss 0x3c00(%r15),%xmm12
    3ca5:	vmovss %xmm12,-0x18(%r9,%r12,4)
    3cac:	vmovss 0x4800(%r15),%xmm12
    3cb5:	vmovss %xmm12,-0x14(%r9,%r12,4)
    3cbc:	vmovss 0x5400(%r15),%xmm12
    3cc5:	vmovss %xmm12,-0x10(%r9,%r12,4)
    3ccc:	vmovss 0x6000(%r15),%xmm12
    3cd5:	vmovss %xmm12,-0xc(%r9,%r12,4)
    3cdc:	vmovss 0x6c00(%r15),%xmm12
    3ce5:	vmovss %xmm12,-0x8(%r9,%r12,4)
    3cec:	vmovss 0x7800(%r15),%xmm12
    3cf5:	vmovss %xmm12,-0x4(%r9,%r12,4)
    3cfc:	vmovss 0x8400(%r15),%xmm12
    3d05:	vmovss %xmm12,(%r9,%r12,4)
    3d0b:	add    $0xc,%r12
    3d0f:	add    $0x9000,%r15
    3d16:	cmp    $0x60,%r12
    3d1a:	jne    3c50 <main_graph_model+0x18a0>
    3d20:	jmp    3c1a <main_graph_model+0x186a>
    3d25:	lea    0xa00(%r14),%rdi
    3d2c:	lea    0x42000(%rax),%rdx
    3d33:	lea    0x47f00(%r14),%r8
    3d3a:	cmp    %r8,%rsi
    3d3d:	setb   %r9b
    3d41:	cmp    %rdx,%rdi
    3d44:	setb   %r8b
    3d48:	and    %r9b,%r8b
    3d4b:	lea    0x3c02c(%rax),%r9
    3d52:	xor    %r10d,%r10d
    3d55:	mov    %rdi,%r11
    3d58:	jmp    3eb2 <main_graph_model+0x1b02>
    3d5d:	nopl   (%rax)
    3d60:	lea    (%rdi,%r10,4),%r15
    3d64:	kxnorb %k0,%k0,%k1
    3d68:	vxorps %xmm12,%xmm12,%xmm12
    3d6d:	vgatherdps (%r15,%ymm0,1),%ymm12{%k1}
    3d74:	kxnorb %k0,%k0,%k1
    3d78:	vxorps %xmm13,%xmm13,%xmm13
    3d7d:	vgatherdps (%r15,%ymm1,1),%ymm13{%k1}
    3d84:	kxnorb %k0,%k0,%k1
    3d88:	vxorps %xmm14,%xmm14,%xmm14
    3d8d:	vgatherdps (%r15,%ymm2,1),%ymm14{%k1}
    3d94:	mov    %r10,%r12
    3d97:	kxnorb %k0,%k0,%k1
    3d9b:	vxorps %xmm15,%xmm15,%xmm15
    3da0:	vgatherdps (%r15,%ymm3,1),%ymm15{%k1}
    3da7:	shl    $0x7,%r12
    3dab:	lea    (%r12,%r12,2),%r12
    3daf:	vmovups %ymm12,(%rsi,%r12,1)
    3db5:	vmovups %ymm13,0x20(%rsi,%r12,1)
    3dbc:	vmovups %ymm14,0x40(%rsi,%r12,1)
    3dc3:	vmovups %ymm15,0x60(%rsi,%r12,1)
    3dca:	kxnorb %k0,%k0,%k1
    3dce:	vxorps %xmm12,%xmm12,%xmm12
    3dd3:	vgatherdps (%r15,%ymm4,1),%ymm12{%k1}
    3dda:	kxnorb %k0,%k0,%k1
    3dde:	vxorps %xmm13,%xmm13,%xmm13
    3de3:	vgatherdps (%r15,%ymm5,1),%ymm13{%k1}
    3dea:	kxnorb %k0,%k0,%k1
    3dee:	vxorps %xmm14,%xmm14,%xmm14
    3df3:	vgatherdps (%r15,%ymm6,1),%ymm14{%k1}
    3dfa:	kxnorb %k0,%k0,%k1
    3dfe:	vxorps %xmm15,%xmm15,%xmm15
    3e03:	vgatherdps (%r15,%ymm7,1),%ymm15{%k1}
    3e0a:	vmovups %ymm12,0x80(%rsi,%r12,1)
    3e14:	vmovups %ymm13,0xa0(%rsi,%r12,1)
    3e1e:	vmovups %ymm14,0xc0(%rsi,%r12,1)
    3e28:	vmovups %ymm15,0xe0(%rsi,%r12,1)
    3e32:	kxnorb %k0,%k0,%k1
    3e36:	vxorps %xmm12,%xmm12,%xmm12
    3e3b:	vgatherdps (%r15,%ymm8,1),%ymm12{%k1}
    3e42:	kxnorb %k0,%k0,%k1
    3e46:	vxorps %xmm13,%xmm13,%xmm13
    3e4b:	vgatherdps (%r15,%ymm9,1),%ymm13{%k1}
    3e52:	kxnorb %k0,%k0,%k1
    3e56:	vxorps %xmm14,%xmm14,%xmm14
    3e5b:	vgatherdps (%r15,%ymm10,1),%ymm14{%k1}
    3e62:	kxnorb %k0,%k0,%k1
    3e66:	vxorps %xmm15,%xmm15,%xmm15
    3e6b:	vgatherdps (%r15,%ymm11,1),%ymm15{%k1}
    3e72:	vmovups %ymm12,0x100(%rsi,%r12,1)
    3e7c:	vmovups %ymm13,0x120(%rsi,%r12,1)
    3e86:	vmovups %ymm14,0x140(%rsi,%r12,1)
    3e90:	vmovups %ymm15,0x160(%rsi,%r12,1)
    3e9a:	inc    %r10
    3e9d:	add    $0x4,%r11
    3ea1:	add    $0x180,%r9
    3ea8:	cmp    $0x40,%r10
    3eac:	je     3fa5 <main_graph_model+0x1bf5>
    3eb2:	test   %r8b,%r8b
    3eb5:	je     3d60 <main_graph_model+0x19b0>
    3ebb:	mov    %r11,%r15
    3ebe:	xor    %r12d,%r12d
    3ec1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    3ed0:	vmovss (%r15),%xmm12
    3ed5:	vmovss %xmm12,-0x2c(%r9,%r12,4)
    3edc:	vmovss 0xc00(%r15),%xmm12
    3ee5:	vmovss %xmm12,-0x28(%r9,%r12,4)
    3eec:	vmovss 0x1800(%r15),%xmm12
    3ef5:	vmovss %xmm12,-0x24(%r9,%r12,4)
    3efc:	vmovss 0x2400(%r15),%xmm12
    3f05:	vmovss %xmm12,-0x20(%r9,%r12,4)
    3f0c:	vmovss 0x3000(%r15),%xmm12
    3f15:	vmovss %xmm12,-0x1c(%r9,%r12,4)
    3f1c:	vmovss 0x3c00(%r15),%xmm12
    3f25:	vmovss %xmm12,-0x18(%r9,%r12,4)
    3f2c:	vmovss 0x4800(%r15),%xmm12
    3f35:	vmovss %xmm12,-0x14(%r9,%r12,4)
    3f3c:	vmovss 0x5400(%r15),%xmm12
    3f45:	vmovss %xmm12,-0x10(%r9,%r12,4)
    3f4c:	vmovss 0x6000(%r15),%xmm12
    3f55:	vmovss %xmm12,-0xc(%r9,%r12,4)
    3f5c:	vmovss 0x6c00(%r15),%xmm12
    3f65:	vmovss %xmm12,-0x8(%r9,%r12,4)
    3f6c:	vmovss 0x7800(%r15),%xmm12
    3f75:	vmovss %xmm12,-0x4(%r9,%r12,4)
    3f7c:	vmovss 0x8400(%r15),%xmm12
    3f85:	vmovss %xmm12,(%r9,%r12,4)
    3f8b:	add    $0xc,%r12
    3f8f:	add    $0x9000,%r15
    3f96:	cmp    $0x60,%r12
    3f9a:	jne    3ed0 <main_graph_model+0x1b20>
    3fa0:	jmp    3e9a <main_graph_model+0x1aea>
    3fa5:	lea    0xb00(%r14),%rsi
    3fac:	lea    0x48000(%rax),%rdi
    3fb3:	add    $0x48000,%r14
    3fba:	cmp    %r14,%rdx
    3fbd:	setb   %r8b
    3fc1:	cmp    %rdi,%rsi
    3fc4:	setb   %dil
    3fc8:	and    %r8b,%dil
    3fcb:	mov    %rax,%r8
    3fce:	add    $0x4202c,%r8
    3fd5:	xor    %r9d,%r9d
    3fd8:	mov    %rsi,%r10
    3fdb:	jmp    4132 <main_graph_model+0x1d82>
    3fe0:	lea    (%rsi,%r9,4),%r11
    3fe4:	kxnorb %k0,%k0,%k1
    3fe8:	vxorps %xmm12,%xmm12,%xmm12
    3fed:	vgatherdps (%r11,%ymm0,1),%ymm12{%k1}
    3ff4:	kxnorb %k0,%k0,%k1
    3ff8:	vxorps %xmm13,%xmm13,%xmm13
    3ffd:	vgatherdps (%r11,%ymm1,1),%ymm13{%k1}
    4004:	kxnorb %k0,%k0,%k1
    4008:	vxorps %xmm14,%xmm14,%xmm14
    400d:	vgatherdps (%r11,%ymm2,1),%ymm14{%k1}
    4014:	mov    %r9,%r14
    4017:	kxnorb %k0,%k0,%k1
    401b:	vxorps %xmm15,%xmm15,%xmm15
    4020:	vgatherdps (%r11,%ymm3,1),%ymm15{%k1}
    4027:	shl    $0x7,%r14
    402b:	lea    (%r14,%r14,2),%r14
    402f:	vmovups %ymm12,(%rdx,%r14,1)
    4035:	vmovups %ymm13,0x20(%rdx,%r14,1)
    403c:	vmovups %ymm14,0x40(%rdx,%r14,1)
    4043:	vmovups %ymm15,0x60(%rdx,%r14,1)
    404a:	kxnorb %k0,%k0,%k1
    404e:	vxorps %xmm12,%xmm12,%xmm12
    4053:	vgatherdps (%r11,%ymm4,1),%ymm12{%k1}
    405a:	kxnorb %k0,%k0,%k1
    405e:	vxorps %xmm13,%xmm13,%xmm13
    4063:	vgatherdps (%r11,%ymm5,1),%ymm13{%k1}
    406a:	kxnorb %k0,%k0,%k1
    406e:	vxorps %xmm14,%xmm14,%xmm14
    4073:	vgatherdps (%r11,%ymm6,1),%ymm14{%k1}
    407a:	kxnorb %k0,%k0,%k1
    407e:	vxorps %xmm15,%xmm15,%xmm15
    4083:	vgatherdps (%r11,%ymm7,1),%ymm15{%k1}
    408a:	vmovups %ymm12,0x80(%rdx,%r14,1)
    4094:	vmovups %ymm13,0xa0(%rdx,%r14,1)
    409e:	vmovups %ymm14,0xc0(%rdx,%r14,1)
    40a8:	vmovups %ymm15,0xe0(%rdx,%r14,1)
    40b2:	kxnorb %k0,%k0,%k1
    40b6:	vxorps %xmm12,%xmm12,%xmm12
    40bb:	vgatherdps (%r11,%ymm8,1),%ymm12{%k1}
    40c2:	kxnorb %k0,%k0,%k1
    40c6:	vxorps %xmm13,%xmm13,%xmm13
    40cb:	vgatherdps (%r11,%ymm9,1),%ymm13{%k1}
    40d2:	kxnorb %k0,%k0,%k1
    40d6:	vxorps %xmm14,%xmm14,%xmm14
    40db:	vgatherdps (%r11,%ymm10,1),%ymm14{%k1}
    40e2:	kxnorb %k0,%k0,%k1
    40e6:	vxorps %xmm15,%xmm15,%xmm15
    40eb:	vgatherdps (%r11,%ymm11,1),%ymm15{%k1}
    40f2:	vmovups %ymm12,0x100(%rdx,%r14,1)
    40fc:	vmovups %ymm13,0x120(%rdx,%r14,1)
    4106:	vmovups %ymm14,0x140(%rdx,%r14,1)
    4110:	vmovups %ymm15,0x160(%rdx,%r14,1)
    411a:	inc    %r9
    411d:	add    $0x4,%r10
    4121:	add    $0x180,%r8
    4128:	cmp    $0x40,%r9
    412c:	je     4225 <main_graph_model+0x1e75>
    4132:	test   %dil,%dil
    4135:	je     3fe0 <main_graph_model+0x1c30>
    413b:	mov    %r10,%r11
    413e:	xor    %r14d,%r14d
    4141:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    4150:	vmovss (%r11),%xmm12
    4155:	vmovss %xmm12,-0x2c(%r8,%r14,4)
    415c:	vmovss 0xc00(%r11),%xmm12
    4165:	vmovss %xmm12,-0x28(%r8,%r14,4)
    416c:	vmovss 0x1800(%r11),%xmm12
    4175:	vmovss %xmm12,-0x24(%r8,%r14,4)
    417c:	vmovss 0x2400(%r11),%xmm12
    4185:	vmovss %xmm12,-0x20(%r8,%r14,4)
    418c:	vmovss 0x3000(%r11),%xmm12
    4195:	vmovss %xmm12,-0x1c(%r8,%r14,4)
    419c:	vmovss 0x3c00(%r11),%xmm12
    41a5:	vmovss %xmm12,-0x18(%r8,%r14,4)
    41ac:	vmovss 0x4800(%r11),%xmm12
    41b5:	vmovss %xmm12,-0x14(%r8,%r14,4)
    41bc:	vmovss 0x5400(%r11),%xmm12
    41c5:	vmovss %xmm12,-0x10(%r8,%r14,4)
    41cc:	vmovss 0x6000(%r11),%xmm12
    41d5:	vmovss %xmm12,-0xc(%r8,%r14,4)
    41dc:	vmovss 0x6c00(%r11),%xmm12
    41e5:	vmovss %xmm12,-0x8(%r8,%r14,4)
    41ec:	vmovss 0x7800(%r11),%xmm12
    41f5:	vmovss %xmm12,-0x4(%r8,%r14,4)
    41fc:	vmovss 0x8400(%r11),%xmm12
    4205:	vmovss %xmm12,(%r8,%r14,4)
    420b:	add    $0xc,%r14
    420f:	add    $0x9000,%r11
    4216:	cmp    $0x60,%r14
    421a:	jne    4150 <main_graph_model+0x1da0>
    4220:	jmp    411a <main_graph_model+0x1d6a>
    4225:	vmovaps 0x2f53(%rip),%ymm0        # 7180 <_fini+0x31c>
    422d:	vmovups %ymm0,0x38(%rbx)
    4232:	vmovaps 0x2f66(%rip),%ymm0        # 71a0 <_fini+0x33c>
    423a:	vmovups %ymm0,0x18(%rbx)
    423f:	mov    %rax,0x8(%rbx)
    4243:	mov    %rcx,(%rbx)
    4246:	movq   $0x0,0x10(%rbx)
    424e:	mov    %rbx,%rax
    4251:	add    $0x8,%rsp
    4255:	pop    %rbx
    4256:	pop    %r12
    4258:	pop    %r14
    425a:	pop    %r15
    425c:	vzeroupper
    425f:	ret
## main_graph_model@plt
    2120:	jmp    *0x7f52(%rip)        # a078 <main_graph_model@@Base+0x7cc8>
    2126:	push   $0xf
    212b:	jmp    2020 <_init+0x20>
## run_main_graph
    4520:	jmp    2070 <run_main_graph_model@plt>
    4525:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    42c0:	push   %rbp
    42c1:	mov    %rsp,%rbp
    42c4:	push   %r15
    42c6:	push   %r14
    42c8:	push   %r13
    42ca:	push   %r12
    42cc:	push   %rbx
    42cd:	sub    $0x48,%rsp
    42d1:	mov    %rdi,%rbx
    42d4:	call   22b0 <omTensorListGetSize@plt>
    42d9:	cmp    $0x1,%rax
    42dd:	jne    44ae <run_main_graph_model+0x1ee>
    42e3:	mov    %rbx,%rdi
    42e6:	call   2150 <omTensorListGetOmtArray@plt>
    42eb:	mov    (%rax),%r14
    42ee:	mov    %r14,%rdi
    42f1:	call   22c0 <omTensorGetDataType@plt>
    42f6:	cmp    $0x1,%rax
    42fa:	jne    44c3 <run_main_graph_model+0x203>
    4300:	mov    %r14,%rdi
    4303:	call   20f0 <omTensorGetRank@plt>
    4308:	cmp    $0x4,%rax
    430c:	jne    44f0 <run_main_graph_model+0x230>
    4312:	mov    %r14,%rdi
    4315:	call   20e0 <omTensorGetShape@plt>
    431a:	mov    (%rax),%rsi
    431d:	cmp    $0x1,%rsi
    4321:	jne    44f9 <run_main_graph_model+0x239>
    4327:	mov    0x8(%rax),%rsi
    432b:	cmp    $0x60,%rsi
    432f:	jne    4502 <run_main_graph_model+0x242>
    4335:	mov    0x10(%rax),%rsi
    4339:	cmp    $0xc,%rsi
    433d:	jne    450b <run_main_graph_model+0x24b>
    4343:	mov    0x18(%rax),%rsi
    4347:	cmp    $0x40,%rsi
    434b:	jne    4514 <run_main_graph_model+0x254>
    4351:	mov    %rbx,%rdi
    4354:	call   2150 <omTensorListGetOmtArray@plt>
    4359:	mov    %rsp,%rbx
    435c:	lea    -0x60(%rbx),%rcx
    4360:	mov    %rcx,%rsp
    4363:	mov    (%rax),%r14
    4366:	mov    %rsp,%r15
    4369:	lea    -0x60(%r15),%rax
    436d:	mov    %rax,-0x30(%rbp)
    4371:	mov    %rax,%rsp
    4374:	mov    %r14,%rdi
    4377:	call   20a0 <omTensorGetDataPtr@plt>
    437c:	mov    %rax,%r12
    437f:	mov    %r14,%rdi
    4382:	call   20e0 <omTensorGetShape@plt>
    4387:	mov    %rax,%r13
    438a:	mov    %r14,%rdi
    438d:	call   2130 <omTensorGetStrides@plt>
    4392:	mov    %r12,-0x60(%r15)
    4396:	mov    %r12,-0x58(%r15)
    439a:	movq   $0x0,-0x50(%r15)
    43a2:	vmovups 0x0(%r13),%ymm0
    43a8:	vmovups %ymm0,-0x48(%r15)
    43ae:	vmovups (%rax),%ymm0
    43b2:	vmovups %ymm0,-0x28(%r15)
    43b8:	lea    -0x60(%rbx),%rdi
    43bc:	mov    -0x30(%rbp),%rsi
    43c0:	vzeroupper
    43c3:	call   2160 <_mlir_ciface_main_graph_model@plt>
    43c8:	mov    -0x60(%rbx),%r15
    43cc:	mov    -0x58(%rbx),%r12
    43d0:	mov    -0x48(%rbx),%rax
    43d4:	mov    %rax,-0x58(%rbp)
    43d8:	mov    -0x40(%rbx),%rax
    43dc:	mov    %rax,-0x60(%rbp)
    43e0:	mov    -0x38(%rbx),%rax
    43e4:	mov    %rax,-0x68(%rbp)
    43e8:	mov    -0x30(%rbx),%rax
    43ec:	mov    %rax,-0x30(%rbp)
    43f0:	mov    -0x28(%rbx),%rax
    43f4:	mov    %rax,-0x38(%rbp)
    43f8:	mov    -0x20(%rbx),%rax
    43fc:	mov    %rax,-0x40(%rbp)
    4400:	mov    -0x18(%rbx),%rax
    4404:	mov    %rax,-0x48(%rbp)
    4408:	mov    -0x10(%rbx),%rax
    440c:	mov    %rax,-0x50(%rbp)
    4410:	mov    %rsp,%r13
    4413:	lea    -0x10(%r13),%rbx
    4417:	mov    %rbx,%rsp
    441a:	mov    $0x4,%edi
    441f:	call   2290 <omTensorCreateUntyped@plt>
    4424:	mov    %rax,%r14
    4427:	mov    $0x1,%esi
    442c:	mov    %rax,%rdi
    442f:	mov    %r15,%rdx
    4432:	mov    %r12,%rcx
    4435:	call   20d0 <omTensorSetDataPtr@plt>
    443a:	mov    $0x1,%esi
    443f:	mov    %r14,%rdi
    4442:	call   22a0 <omTensorSetDataType@plt>
    4447:	mov    %r14,%rdi
    444a:	call   20e0 <omTensorGetShape@plt>
    444f:	mov    %rax,%r15
    4452:	mov    %r14,%rdi
    4455:	call   2130 <omTensorGetStrides@plt>
    445a:	mov    -0x58(%rbp),%rcx
    445e:	mov    %rcx,(%r15)
    4461:	mov    -0x38(%rbp),%rcx
    4465:	mov    %rcx,(%rax)
    4468:	mov    -0x60(%rbp),%rcx
    446c:	mov    %rcx,0x8(%r15)
    4470:	mov    -0x40(%rbp),%rcx
    4474:	mov    %rcx,0x8(%rax)
    4478:	mov    -0x68(%rbp),%rcx
    447c:	mov    %rcx,0x10(%r15)
    4480:	mov    -0x48(%rbp),%rcx
    4484:	mov    %rcx,0x10(%rax)
    4488:	mov    -0x30(%rbp),%rcx
    448c:	mov    %rcx,0x18(%r15)
    4490:	mov    -0x50(%rbp),%rcx
    4494:	mov    %rcx,0x18(%rax)
    4498:	mov    %r14,-0x10(%r13)
    449c:	mov    $0x1,%esi
    44a1:	mov    %rbx,%rdi
    44a4:	call   21d0 <omTensorListCreate@plt>
    44a9:	mov    %rax,%rbx
    44ac:	jmp    44de <run_main_graph_model+0x21e>
    44ae:	lea    0x2f9b(%rip),%rdi        # 7450 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    44b5:	xor    %ebx,%ebx
    44b7:	mov    %rax,%rsi
    44ba:	xor    %eax,%eax
    44bc:	call   2040 <printf@plt>
    44c1:	jmp    44d3 <run_main_graph_model+0x213>
    44c3:	lea    0x2f56(%rip),%rdi        # 7420 <om_Wrong data type for the input 0: expect f32^J_model>
    44ca:	xor    %ebx,%ebx
    44cc:	xor    %eax,%eax
    44ce:	call   2040 <printf@plt>
    44d3:	call   2030 <__errno_location@plt>
    44d8:	movl   $0x16,(%rax)
    44de:	mov    %rbx,%rax
    44e1:	lea    -0x28(%rbp),%rsp
    44e5:	pop    %rbx
    44e6:	pop    %r12
    44e8:	pop    %r13
    44ea:	pop    %r14
    44ec:	pop    %r15
    44ee:	pop    %rbp
    44ef:	ret
    44f0:	lea    0x2ee9(%rip),%rdi        # 73e0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    44f7:	jmp    44b5 <run_main_graph_model+0x1f5>
    44f9:	lea    0x2e90(%rip),%rdi        # 7390 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    4500:	jmp    451b <run_main_graph_model+0x25b>
    4502:	lea    0x2e37(%rip),%rdi        # 7340 <om_Wrong size for the dimension 1 of the input 0: expect 96, but got %lld^J_model>
    4509:	jmp    451b <run_main_graph_model+0x25b>
    450b:	lea    0x2dde(%rip),%rdi        # 72f0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    4512:	jmp    451b <run_main_graph_model+0x25b>
    4514:	lea    0x2d85(%rip),%rdi        # 72a0 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    451b:	xor    %ebx,%ebx
    451d:	jmp    44ba <run_main_graph_model+0x1fa>
    451f:	nop
## run_main_graph_model@plt
    2070:	jmp    *0x7faa(%rip)        # a020 <run_main_graph_model@@Base+0x5d60>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
