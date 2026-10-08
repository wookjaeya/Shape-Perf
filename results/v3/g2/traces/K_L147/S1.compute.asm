## _mlir_ciface_main_graph_model
    4bd0:	push   %rbx
    4bd1:	sub    $0x90,%rsp
    4bd8:	mov    %rdi,%rbx
    4bdb:	mov    0x8(%rsi),%rdx
    4bdf:	lea    0x38(%rsp),%rdi
    4be4:	call   2120 <main_graph_model@plt>
    4be9:	mov    0x88(%rsp),%rax
    4bf1:	vmovups 0x78(%rsp),%xmm0
    4bf7:	vmovups 0x38(%rsp),%ymm1
    4bfd:	vmovups 0x58(%rsp),%ymm2
    4c03:	vmovups %ymm1,(%rbx)
    4c07:	vmovups %ymm2,0x20(%rbx)
    4c0c:	vmovups %xmm0,0x40(%rbx)
    4c11:	mov    %rax,0x50(%rbx)
    4c15:	add    $0x90,%rsp
    4c1c:	pop    %rbx
    4c1d:	vzeroupper
    4c20:	ret
    4c21:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2160:	jmp    *0x8f32(%rip)        # b098 <_mlir_ciface_main_graph_model@@Base+0x64c8>
    2166:	push   $0x13
    216b:	jmp    2020 <_init+0x20>
## main_graph_model
    23b0:	push   %rbp
    23b1:	push   %r15
    23b3:	push   %r14
    23b5:	push   %r13
    23b7:	push   %r12
    23b9:	push   %rbx
    23ba:	sub    $0x18,%rsp
    23be:	mov    %rdx,%r14
    23c1:	mov    %rdi,0x10(%rsp)
    23c6:	mov    $0x6e410,%edi
    23cb:	call   21e0 <malloc@plt>
    23d0:	mov    %rax,0x8(%rsp)
    23d5:	add    $0xf,%rax
    23d9:	and    $0xfffffffffffffff0,%rax
    23dd:	lea    0x9300(%rax),%rdx
    23e4:	lea    0x6d900(%r14),%rcx
    23eb:	cmp    %rax,%rcx
    23ee:	setbe  %cl
    23f1:	cmp    %rdx,%r14
    23f4:	setae  %sil
    23f8:	or     %cl,%sil
    23fb:	lea    0x6000(%r14),%rdi
    2402:	xor    %r8d,%r8d
    2405:	vmovaps 0x5bf3(%rip),%ymm0        # 8000 <_fini+0x81c>
    240d:	vmovaps 0x5c0b(%rip),%ymm1        # 8020 <_fini+0x83c>
    2415:	vmovaps 0x5c23(%rip),%ymm2        # 8040 <_fini+0x85c>
    241d:	vmovaps 0x5c3b(%rip),%ymm3        # 8060 <_fini+0x87c>
    2425:	vmovaps 0x5c53(%rip),%ymm4        # 8080 <_fini+0x89c>
    242d:	vmovaps 0x5c6b(%rip),%ymm5        # 80a0 <_fini+0x8bc>
    2435:	vmovaps 0x5c83(%rip),%ymm6        # 80c0 <_fini+0x8dc>
    243d:	vmovaps 0x5c9b(%rip),%ymm7        # 80e0 <_fini+0x8fc>
    2445:	vmovaps 0x5cb3(%rip),%ymm8        # 8100 <_fini+0x91c>
    244d:	vmovaps 0x5ccb(%rip),%ymm9        # 8120 <_fini+0x93c>
    2455:	vmovaps 0x5ce3(%rip),%ymm10        # 8140 <_fini+0x95c>
    245d:	vmovaps 0x5cfb(%rip),%ymm11        # 8160 <_fini+0x97c>
    2465:	vmovaps 0x5d13(%rip),%ymm12        # 8180 <_fini+0x99c>
    246d:	vmovaps 0x5d2b(%rip),%ymm13        # 81a0 <_fini+0x9bc>
    2475:	vmovaps 0x5d43(%rip),%ymm14        # 81c0 <_fini+0x9dc>
    247d:	vmovaps 0x5d5b(%rip),%ymm15        # 81e0 <_fini+0x9fc>
    2485:	vmovaps 0x5d71(%rip),%ymm16        # 8200 <_fini+0xa1c>
    248f:	vmovaps 0x5d87(%rip),%ymm17        # 8220 <_fini+0xa3c>
    2499:	mov    %r14,%r9
    249c:	mov    %rax,%r10
    249f:	jmp    24cc <main_graph_model+0x11c>
    24a1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    24b0:	inc    %r8
    24b3:	add    $0x24c,%r10
    24ba:	add    $0x4,%r9
    24be:	add    $0x4,%rdi
    24c2:	cmp    $0x40,%r8
    24c6:	je     27b8 <main_graph_model+0x408>
    24cc:	test   %sil,%sil
    24cf:	je     26b0 <main_graph_model+0x300>
    24d5:	lea    (%r14,%r8,4),%r11
    24d9:	kxnorb %k0,%k0,%k1
    24dd:	vxorps %xmm18,%xmm18,%xmm18
    24e3:	vgatherdps (%r11,%ymm0,1),%ymm18{%k1}
    24ea:	kxnorb %k0,%k0,%k1
    24ee:	vxorps %xmm19,%xmm19,%xmm19
    24f4:	vgatherdps (%r11,%ymm1,1),%ymm19{%k1}
    24fb:	kxnorb %k0,%k0,%k1
    24ff:	vxorps %xmm20,%xmm20,%xmm20
    2505:	vgatherdps (%r11,%ymm2,1),%ymm20{%k1}
    250c:	kxnorb %k0,%k0,%k1
    2510:	vxorps %xmm21,%xmm21,%xmm21
    2516:	vgatherdps (%r11,%ymm3,1),%ymm21{%k1}
    251d:	imul   $0x24c,%r8,%r15
    2524:	vmovups %ymm18,(%rax,%r15,1)
    252b:	vmovups %ymm19,0x20(%rax,%r15,1)
    2533:	vmovups %ymm20,0x40(%rax,%r15,1)
    253b:	vmovups %ymm21,0x60(%rax,%r15,1)
    2543:	kxnorb %k0,%k0,%k1
    2547:	vxorps %xmm18,%xmm18,%xmm18
    254d:	vgatherdps (%r11,%ymm4,1),%ymm18{%k1}
    2554:	kxnorb %k0,%k0,%k1
    2558:	vxorps %xmm19,%xmm19,%xmm19
    255e:	vgatherdps (%r11,%ymm5,1),%ymm19{%k1}
    2565:	kxnorb %k0,%k0,%k1
    2569:	vxorps %xmm20,%xmm20,%xmm20
    256f:	vgatherdps (%r11,%ymm6,1),%ymm20{%k1}
    2576:	kxnorb %k0,%k0,%k1
    257a:	vxorps %xmm21,%xmm21,%xmm21
    2580:	vgatherdps (%r11,%ymm7,1),%ymm21{%k1}
    2587:	vmovups %ymm18,0x80(%rax,%r15,1)
    258f:	vmovups %ymm19,0xa0(%rax,%r15,1)
    2597:	vmovups %ymm20,0xc0(%rax,%r15,1)
    259f:	vmovups %ymm21,0xe0(%rax,%r15,1)
    25a7:	kxnorb %k0,%k0,%k1
    25ab:	vxorps %xmm18,%xmm18,%xmm18
    25b1:	vgatherdps (%r11,%ymm8,1),%ymm18{%k1}
    25b8:	kxnorb %k0,%k0,%k1
    25bc:	vxorps %xmm19,%xmm19,%xmm19
    25c2:	vgatherdps (%r11,%ymm9,1),%ymm19{%k1}
    25c9:	kxnorb %k0,%k0,%k1
    25cd:	vxorps %xmm20,%xmm20,%xmm20
    25d3:	vgatherdps (%r11,%ymm10,1),%ymm20{%k1}
    25da:	kxnorb %k0,%k0,%k1
    25de:	vxorps %xmm21,%xmm21,%xmm21
    25e4:	vgatherdps (%r11,%ymm11,1),%ymm21{%k1}
    25eb:	vmovups %ymm18,0x100(%rax,%r15,1)
    25f3:	vmovups %ymm19,0x120(%rax,%r15,1)
    25fb:	vmovups %ymm20,0x140(%rax,%r15,1)
    2603:	vmovups %ymm21,0x160(%rax,%r15,1)
    260b:	kxnorb %k0,%k0,%k1
    260f:	vxorps %xmm18,%xmm18,%xmm18
    2615:	vgatherdps (%r11,%ymm12,1),%ymm18{%k1}
    261c:	kxnorb %k0,%k0,%k1
    2620:	vxorps %xmm19,%xmm19,%xmm19
    2626:	vgatherdps (%r11,%ymm13,1),%ymm19{%k1}
    262d:	kxnorb %k0,%k0,%k1
    2631:	vxorps %xmm20,%xmm20,%xmm20
    2637:	vgatherdps (%r11,%ymm14,1),%ymm20{%k1}
    263e:	kxnorb %k0,%k0,%k1
    2642:	vxorps %xmm21,%xmm21,%xmm21
    2648:	vgatherdps (%r11,%ymm15,1),%ymm21{%k1}
    264f:	vmovups %ymm18,0x180(%rax,%r15,1)
    2657:	vmovups %ymm19,0x1a0(%rax,%r15,1)
    265f:	vmovups %ymm20,0x1c0(%rax,%r15,1)
    2667:	vmovups %ymm21,0x1e0(%rax,%r15,1)
    266f:	kxnorb %k0,%k0,%k1
    2673:	vxorps %xmm18,%xmm18,%xmm18
    2679:	vgatherdps (%r11,%ymm16,1),%ymm18{%k1}
    2680:	vmovups %ymm18,0x200(%rax,%r15,1)
    2688:	kxnorb %k0,%k0,%k1
    268c:	vxorps %xmm18,%xmm18,%xmm18
    2692:	vgatherdps (%r11,%ymm17,1),%ymm18{%k1}
    2699:	vmovups %ymm18,0x220(%rax,%r15,1)
    26a1:	mov    $0x90,%r11d
    26a7:	jmp    26b3 <main_graph_model+0x303>
    26a9:	nopl   0x0(%rax)
    26b0:	xor    %r11d,%r11d
    26b3:	mov    %r11d,%ecx
    26b6:	shl    $0xa,%ecx
    26b9:	lea    (%rcx,%rcx,2),%r13
    26bd:	lea    (%r9,%r13,1),%r12
    26c1:	add    %rdi,%r13
    26c4:	xor    %ecx,%ecx
    26c6:	cs nopw 0x0(%rax,%rax,1)
    26d0:	mov    %r13,%r15
    26d3:	vmovss (%r12,%rcx,4),%xmm18
    26da:	vmovss %xmm18,(%r10,%r11,4)
    26e1:	inc    %r11
    26e4:	add    $0x300,%rcx
    26eb:	add    $0xc00,%r13
    26f2:	cmp    $0x900,%rcx
    26f9:	jne    26d0 <main_graph_model+0x320>
    26fb:	test   %sil,%sil
    26fe:	jne    24b0 <main_graph_model+0x100>
    2704:	data16 data16 cs nopw 0x0(%rax,%rax,1)
    2710:	vmovss -0x5400(%r15),%xmm18
    271a:	vmovss %xmm18,(%r10,%r11,4)
    2721:	vmovss -0x4800(%r15),%xmm18
    272b:	vmovss %xmm18,0x4(%r10,%r11,4)
    2733:	vmovss -0x3c00(%r15),%xmm18
    273d:	vmovss %xmm18,0x8(%r10,%r11,4)
    2745:	vmovss -0x3000(%r15),%xmm18
    274f:	vmovss %xmm18,0xc(%r10,%r11,4)
    2757:	vmovss -0x2400(%r15),%xmm18
    2761:	vmovss %xmm18,0x10(%r10,%r11,4)
    2769:	vmovss -0x1800(%r15),%xmm18
    2773:	vmovss %xmm18,0x14(%r10,%r11,4)
    277b:	vmovss -0xc00(%r15),%xmm18
    2785:	vmovss %xmm18,0x18(%r10,%r11,4)
    278d:	vmovss (%r15),%xmm18
    2793:	vmovss %xmm18,0x1c(%r10,%r11,4)
    279b:	add    $0x8,%r11
    279f:	add    $0x6000,%r15
    27a6:	cmp    $0x93,%r11
    27ad:	jne    2710 <main_graph_model+0x360>
    27b3:	jmp    24b0 <main_graph_model+0x100>
    27b8:	lea    0x100(%r14),%rdi
    27bf:	lea    0x12600(%rax),%rsi
    27c6:	lea    0x6da00(%r14),%rcx
    27cd:	cmp    %rcx,%rdx
    27d0:	setae  %cl
    27d3:	cmp    %rsi,%rdi
    27d6:	setae  %r8b
    27da:	or     %cl,%r8b
    27dd:	lea    0x3d00(%r14),%r9
    27e4:	xor    %r10d,%r10d
    27e7:	mov    %r14,%r11
    27ea:	mov    %rdx,%r15
    27ed:	jmp    280c <main_graph_model+0x45c>
    27ef:	nop
    27f0:	inc    %r10
    27f3:	add    $0x24c,%r15
    27fa:	add    $0x4,%r11
    27fe:	add    $0x4,%r9
    2802:	cmp    $0x40,%r10
    2806:	je     2af9 <main_graph_model+0x749>
    280c:	test   %r8b,%r8b
    280f:	je     29f0 <main_graph_model+0x640>
    2815:	lea    (%rdi,%r10,4),%r12
    2819:	kxnorb %k0,%k0,%k1
    281d:	vxorps %xmm18,%xmm18,%xmm18
    2823:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    282a:	kxnorb %k0,%k0,%k1
    282e:	vxorps %xmm19,%xmm19,%xmm19
    2834:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    283b:	kxnorb %k0,%k0,%k1
    283f:	vxorps %xmm20,%xmm20,%xmm20
    2845:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    284c:	kxnorb %k0,%k0,%k1
    2850:	vxorps %xmm21,%xmm21,%xmm21
    2856:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    285d:	imul   $0x24c,%r10,%r13
    2864:	vmovups %ymm18,(%rdx,%r13,1)
    286b:	vmovups %ymm19,0x20(%rdx,%r13,1)
    2873:	vmovups %ymm20,0x40(%rdx,%r13,1)
    287b:	vmovups %ymm21,0x60(%rdx,%r13,1)
    2883:	kxnorb %k0,%k0,%k1
    2887:	vxorps %xmm18,%xmm18,%xmm18
    288d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    2894:	kxnorb %k0,%k0,%k1
    2898:	vxorps %xmm19,%xmm19,%xmm19
    289e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    28a5:	kxnorb %k0,%k0,%k1
    28a9:	vxorps %xmm20,%xmm20,%xmm20
    28af:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    28b6:	kxnorb %k0,%k0,%k1
    28ba:	vxorps %xmm21,%xmm21,%xmm21
    28c0:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    28c7:	vmovups %ymm18,0x80(%rdx,%r13,1)
    28cf:	vmovups %ymm19,0xa0(%rdx,%r13,1)
    28d7:	vmovups %ymm20,0xc0(%rdx,%r13,1)
    28df:	vmovups %ymm21,0xe0(%rdx,%r13,1)
    28e7:	kxnorb %k0,%k0,%k1
    28eb:	vxorps %xmm18,%xmm18,%xmm18
    28f1:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    28f8:	kxnorb %k0,%k0,%k1
    28fc:	vxorps %xmm19,%xmm19,%xmm19
    2902:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    2909:	kxnorb %k0,%k0,%k1
    290d:	vxorps %xmm20,%xmm20,%xmm20
    2913:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    291a:	kxnorb %k0,%k0,%k1
    291e:	vxorps %xmm21,%xmm21,%xmm21
    2924:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    292b:	vmovups %ymm18,0x100(%rdx,%r13,1)
    2933:	vmovups %ymm19,0x120(%rdx,%r13,1)
    293b:	vmovups %ymm20,0x140(%rdx,%r13,1)
    2943:	vmovups %ymm21,0x160(%rdx,%r13,1)
    294b:	kxnorb %k0,%k0,%k1
    294f:	vxorps %xmm18,%xmm18,%xmm18
    2955:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    295c:	kxnorb %k0,%k0,%k1
    2960:	vxorps %xmm19,%xmm19,%xmm19
    2966:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    296d:	kxnorb %k0,%k0,%k1
    2971:	vxorps %xmm20,%xmm20,%xmm20
    2977:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    297e:	kxnorb %k0,%k0,%k1
    2982:	vxorps %xmm21,%xmm21,%xmm21
    2988:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    298f:	vmovups %ymm18,0x180(%rdx,%r13,1)
    2997:	vmovups %ymm19,0x1a0(%rdx,%r13,1)
    299f:	vmovups %ymm20,0x1c0(%rdx,%r13,1)
    29a7:	vmovups %ymm21,0x1e0(%rdx,%r13,1)
    29af:	kxnorb %k0,%k0,%k1
    29b3:	vxorps %xmm18,%xmm18,%xmm18
    29b9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    29c0:	vmovups %ymm18,0x200(%rdx,%r13,1)
    29c8:	kxnorb %k0,%k0,%k1
    29cc:	vxorps %xmm18,%xmm18,%xmm18
    29d2:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    29d9:	vmovups %ymm18,0x220(%rdx,%r13,1)
    29e1:	mov    $0x90,%r12d
    29e7:	jmp    29f3 <main_graph_model+0x643>
    29e9:	nopl   0x0(%rax)
    29f0:	xor    %r12d,%r12d
    29f3:	mov    %r12d,%ecx
    29f6:	shl    $0xa,%ecx
    29f9:	lea    (%rcx,%rcx,2),%rbx
    29fd:	lea    (%r11,%rbx,1),%rbp
    2a01:	add    %r9,%rbx
    2a04:	mov    $0x40,%ecx
    2a09:	nopl   0x0(%rax)
    2a10:	mov    %rbx,%r13
    2a13:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    2a1b:	vmovss %xmm18,(%r15,%r12,4)
    2a22:	inc    %r12
    2a25:	add    $0x300,%rcx
    2a2c:	add    $0xc00,%rbx
    2a33:	cmp    $0x940,%rcx
    2a3a:	jne    2a10 <main_graph_model+0x660>
    2a3c:	test   %r8b,%r8b
    2a3f:	jne    27f0 <main_graph_model+0x440>
    2a45:	data16 cs nopw 0x0(%rax,%rax,1)
    2a50:	vmovss -0x3000(%r13),%xmm18
    2a5a:	vmovss %xmm18,(%r15,%r12,4)
    2a61:	vmovss -0x2400(%r13),%xmm18
    2a6b:	vmovss %xmm18,0x4(%r15,%r12,4)
    2a73:	vmovss -0x1800(%r13),%xmm18
    2a7d:	vmovss %xmm18,0x8(%r15,%r12,4)
    2a85:	vmovss -0xc00(%r13),%xmm18
    2a8f:	vmovss %xmm18,0xc(%r15,%r12,4)
    2a97:	vmovss 0x0(%r13),%xmm18
    2a9e:	vmovss %xmm18,0x10(%r15,%r12,4)
    2aa6:	vmovss 0xc00(%r13),%xmm18
    2ab0:	vmovss %xmm18,0x14(%r15,%r12,4)
    2ab8:	vmovss 0x1800(%r13),%xmm18
    2ac2:	vmovss %xmm18,0x18(%r15,%r12,4)
    2aca:	vmovss 0x2400(%r13),%xmm18
    2ad4:	vmovss %xmm18,0x1c(%r15,%r12,4)
    2adc:	add    $0x8,%r12
    2ae0:	add    $0x6000,%r13
    2ae7:	cmp    $0x93,%r12
    2aee:	jne    2a50 <main_graph_model+0x6a0>
    2af4:	jmp    27f0 <main_graph_model+0x440>
    2af9:	lea    0x200(%r14),%rdi
    2b00:	lea    0x1b900(%rax),%rdx
    2b07:	lea    0x6db00(%r14),%rcx
    2b0e:	cmp    %rcx,%rsi
    2b11:	setae  %cl
    2b14:	cmp    %rdx,%rdi
    2b17:	setae  %r8b
    2b1b:	or     %cl,%r8b
    2b1e:	lea    0x3e00(%r14),%r9
    2b25:	xor    %r10d,%r10d
    2b28:	mov    %r14,%r11
    2b2b:	mov    %rsi,%r15
    2b2e:	jmp    2b4c <main_graph_model+0x79c>
    2b30:	inc    %r10
    2b33:	add    $0x24c,%r15
    2b3a:	add    $0x4,%r11
    2b3e:	add    $0x4,%r9
    2b42:	cmp    $0x40,%r10
    2b46:	je     2e39 <main_graph_model+0xa89>
    2b4c:	test   %r8b,%r8b
    2b4f:	je     2d30 <main_graph_model+0x980>
    2b55:	lea    (%rdi,%r10,4),%r12
    2b59:	kxnorb %k0,%k0,%k1
    2b5d:	vxorps %xmm18,%xmm18,%xmm18
    2b63:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    2b6a:	kxnorb %k0,%k0,%k1
    2b6e:	vxorps %xmm19,%xmm19,%xmm19
    2b74:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    2b7b:	kxnorb %k0,%k0,%k1
    2b7f:	vxorps %xmm20,%xmm20,%xmm20
    2b85:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    2b8c:	kxnorb %k0,%k0,%k1
    2b90:	vxorps %xmm21,%xmm21,%xmm21
    2b96:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    2b9d:	imul   $0x24c,%r10,%r13
    2ba4:	vmovups %ymm18,(%rsi,%r13,1)
    2bab:	vmovups %ymm19,0x20(%rsi,%r13,1)
    2bb3:	vmovups %ymm20,0x40(%rsi,%r13,1)
    2bbb:	vmovups %ymm21,0x60(%rsi,%r13,1)
    2bc3:	kxnorb %k0,%k0,%k1
    2bc7:	vxorps %xmm18,%xmm18,%xmm18
    2bcd:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    2bd4:	kxnorb %k0,%k0,%k1
    2bd8:	vxorps %xmm19,%xmm19,%xmm19
    2bde:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    2be5:	kxnorb %k0,%k0,%k1
    2be9:	vxorps %xmm20,%xmm20,%xmm20
    2bef:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    2bf6:	kxnorb %k0,%k0,%k1
    2bfa:	vxorps %xmm21,%xmm21,%xmm21
    2c00:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    2c07:	vmovups %ymm18,0x80(%rsi,%r13,1)
    2c0f:	vmovups %ymm19,0xa0(%rsi,%r13,1)
    2c17:	vmovups %ymm20,0xc0(%rsi,%r13,1)
    2c1f:	vmovups %ymm21,0xe0(%rsi,%r13,1)
    2c27:	kxnorb %k0,%k0,%k1
    2c2b:	vxorps %xmm18,%xmm18,%xmm18
    2c31:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    2c38:	kxnorb %k0,%k0,%k1
    2c3c:	vxorps %xmm19,%xmm19,%xmm19
    2c42:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    2c49:	kxnorb %k0,%k0,%k1
    2c4d:	vxorps %xmm20,%xmm20,%xmm20
    2c53:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    2c5a:	kxnorb %k0,%k0,%k1
    2c5e:	vxorps %xmm21,%xmm21,%xmm21
    2c64:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    2c6b:	vmovups %ymm18,0x100(%rsi,%r13,1)
    2c73:	vmovups %ymm19,0x120(%rsi,%r13,1)
    2c7b:	vmovups %ymm20,0x140(%rsi,%r13,1)
    2c83:	vmovups %ymm21,0x160(%rsi,%r13,1)
    2c8b:	kxnorb %k0,%k0,%k1
    2c8f:	vxorps %xmm18,%xmm18,%xmm18
    2c95:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    2c9c:	kxnorb %k0,%k0,%k1
    2ca0:	vxorps %xmm19,%xmm19,%xmm19
    2ca6:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    2cad:	kxnorb %k0,%k0,%k1
    2cb1:	vxorps %xmm20,%xmm20,%xmm20
    2cb7:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    2cbe:	kxnorb %k0,%k0,%k1
    2cc2:	vxorps %xmm21,%xmm21,%xmm21
    2cc8:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    2ccf:	vmovups %ymm18,0x180(%rsi,%r13,1)
    2cd7:	vmovups %ymm19,0x1a0(%rsi,%r13,1)
    2cdf:	vmovups %ymm20,0x1c0(%rsi,%r13,1)
    2ce7:	vmovups %ymm21,0x1e0(%rsi,%r13,1)
    2cef:	kxnorb %k0,%k0,%k1
    2cf3:	vxorps %xmm18,%xmm18,%xmm18
    2cf9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    2d00:	vmovups %ymm18,0x200(%rsi,%r13,1)
    2d08:	kxnorb %k0,%k0,%k1
    2d0c:	vxorps %xmm18,%xmm18,%xmm18
    2d12:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    2d19:	vmovups %ymm18,0x220(%rsi,%r13,1)
    2d21:	mov    $0x90,%r12d
    2d27:	jmp    2d33 <main_graph_model+0x983>
    2d29:	nopl   0x0(%rax)
    2d30:	xor    %r12d,%r12d
    2d33:	mov    %r12d,%ecx
    2d36:	shl    $0xa,%ecx
    2d39:	lea    (%rcx,%rcx,2),%rbx
    2d3d:	lea    (%r11,%rbx,1),%rbp
    2d41:	add    %r9,%rbx
    2d44:	mov    $0x80,%ecx
    2d49:	nopl   0x0(%rax)
    2d50:	mov    %rbx,%r13
    2d53:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    2d5b:	vmovss %xmm18,(%r15,%r12,4)
    2d62:	inc    %r12
    2d65:	add    $0x300,%rcx
    2d6c:	add    $0xc00,%rbx
    2d73:	cmp    $0x980,%rcx
    2d7a:	jne    2d50 <main_graph_model+0x9a0>
    2d7c:	test   %r8b,%r8b
    2d7f:	jne    2b30 <main_graph_model+0x780>
    2d85:	data16 cs nopw 0x0(%rax,%rax,1)
    2d90:	vmovss -0x3000(%r13),%xmm18
    2d9a:	vmovss %xmm18,(%r15,%r12,4)
    2da1:	vmovss -0x2400(%r13),%xmm18
    2dab:	vmovss %xmm18,0x4(%r15,%r12,4)
    2db3:	vmovss -0x1800(%r13),%xmm18
    2dbd:	vmovss %xmm18,0x8(%r15,%r12,4)
    2dc5:	vmovss -0xc00(%r13),%xmm18
    2dcf:	vmovss %xmm18,0xc(%r15,%r12,4)
    2dd7:	vmovss 0x0(%r13),%xmm18
    2dde:	vmovss %xmm18,0x10(%r15,%r12,4)
    2de6:	vmovss 0xc00(%r13),%xmm18
    2df0:	vmovss %xmm18,0x14(%r15,%r12,4)
    2df8:	vmovss 0x1800(%r13),%xmm18
    2e02:	vmovss %xmm18,0x18(%r15,%r12,4)
    2e0a:	vmovss 0x2400(%r13),%xmm18
    2e14:	vmovss %xmm18,0x1c(%r15,%r12,4)
    2e1c:	add    $0x8,%r12
    2e20:	add    $0x6000,%r13
    2e27:	cmp    $0x93,%r12
    2e2e:	jne    2d90 <main_graph_model+0x9e0>
    2e34:	jmp    2b30 <main_graph_model+0x780>
    2e39:	lea    0x300(%r14),%rdi
    2e40:	lea    0x24c00(%rax),%rsi
    2e47:	lea    0x6dc00(%r14),%rcx
    2e4e:	cmp    %rcx,%rdx
    2e51:	setae  %cl
    2e54:	cmp    %rsi,%rdi
    2e57:	setae  %r8b
    2e5b:	or     %cl,%r8b
    2e5e:	lea    0x3f00(%r14),%r9
    2e65:	xor    %r10d,%r10d
    2e68:	mov    %r14,%r11
    2e6b:	mov    %rdx,%r15
    2e6e:	jmp    2e8c <main_graph_model+0xadc>
    2e70:	inc    %r10
    2e73:	add    $0x24c,%r15
    2e7a:	add    $0x4,%r11
    2e7e:	add    $0x4,%r9
    2e82:	cmp    $0x40,%r10
    2e86:	je     3179 <main_graph_model+0xdc9>
    2e8c:	test   %r8b,%r8b
    2e8f:	je     3070 <main_graph_model+0xcc0>
    2e95:	lea    (%rdi,%r10,4),%r12
    2e99:	kxnorb %k0,%k0,%k1
    2e9d:	vxorps %xmm18,%xmm18,%xmm18
    2ea3:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    2eaa:	kxnorb %k0,%k0,%k1
    2eae:	vxorps %xmm19,%xmm19,%xmm19
    2eb4:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    2ebb:	kxnorb %k0,%k0,%k1
    2ebf:	vxorps %xmm20,%xmm20,%xmm20
    2ec5:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    2ecc:	kxnorb %k0,%k0,%k1
    2ed0:	vxorps %xmm21,%xmm21,%xmm21
    2ed6:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    2edd:	imul   $0x24c,%r10,%r13
    2ee4:	vmovups %ymm18,(%rdx,%r13,1)
    2eeb:	vmovups %ymm19,0x20(%rdx,%r13,1)
    2ef3:	vmovups %ymm20,0x40(%rdx,%r13,1)
    2efb:	vmovups %ymm21,0x60(%rdx,%r13,1)
    2f03:	kxnorb %k0,%k0,%k1
    2f07:	vxorps %xmm18,%xmm18,%xmm18
    2f0d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    2f14:	kxnorb %k0,%k0,%k1
    2f18:	vxorps %xmm19,%xmm19,%xmm19
    2f1e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    2f25:	kxnorb %k0,%k0,%k1
    2f29:	vxorps %xmm20,%xmm20,%xmm20
    2f2f:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    2f36:	kxnorb %k0,%k0,%k1
    2f3a:	vxorps %xmm21,%xmm21,%xmm21
    2f40:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    2f47:	vmovups %ymm18,0x80(%rdx,%r13,1)
    2f4f:	vmovups %ymm19,0xa0(%rdx,%r13,1)
    2f57:	vmovups %ymm20,0xc0(%rdx,%r13,1)
    2f5f:	vmovups %ymm21,0xe0(%rdx,%r13,1)
    2f67:	kxnorb %k0,%k0,%k1
    2f6b:	vxorps %xmm18,%xmm18,%xmm18
    2f71:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    2f78:	kxnorb %k0,%k0,%k1
    2f7c:	vxorps %xmm19,%xmm19,%xmm19
    2f82:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    2f89:	kxnorb %k0,%k0,%k1
    2f8d:	vxorps %xmm20,%xmm20,%xmm20
    2f93:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    2f9a:	kxnorb %k0,%k0,%k1
    2f9e:	vxorps %xmm21,%xmm21,%xmm21
    2fa4:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    2fab:	vmovups %ymm18,0x100(%rdx,%r13,1)
    2fb3:	vmovups %ymm19,0x120(%rdx,%r13,1)
    2fbb:	vmovups %ymm20,0x140(%rdx,%r13,1)
    2fc3:	vmovups %ymm21,0x160(%rdx,%r13,1)
    2fcb:	kxnorb %k0,%k0,%k1
    2fcf:	vxorps %xmm18,%xmm18,%xmm18
    2fd5:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    2fdc:	kxnorb %k0,%k0,%k1
    2fe0:	vxorps %xmm19,%xmm19,%xmm19
    2fe6:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    2fed:	kxnorb %k0,%k0,%k1
    2ff1:	vxorps %xmm20,%xmm20,%xmm20
    2ff7:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    2ffe:	kxnorb %k0,%k0,%k1
    3002:	vxorps %xmm21,%xmm21,%xmm21
    3008:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    300f:	vmovups %ymm18,0x180(%rdx,%r13,1)
    3017:	vmovups %ymm19,0x1a0(%rdx,%r13,1)
    301f:	vmovups %ymm20,0x1c0(%rdx,%r13,1)
    3027:	vmovups %ymm21,0x1e0(%rdx,%r13,1)
    302f:	kxnorb %k0,%k0,%k1
    3033:	vxorps %xmm18,%xmm18,%xmm18
    3039:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    3040:	vmovups %ymm18,0x200(%rdx,%r13,1)
    3048:	kxnorb %k0,%k0,%k1
    304c:	vxorps %xmm18,%xmm18,%xmm18
    3052:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    3059:	vmovups %ymm18,0x220(%rdx,%r13,1)
    3061:	mov    $0x90,%r12d
    3067:	jmp    3073 <main_graph_model+0xcc3>
    3069:	nopl   0x0(%rax)
    3070:	xor    %r12d,%r12d
    3073:	mov    %r12d,%ecx
    3076:	shl    $0xa,%ecx
    3079:	lea    (%rcx,%rcx,2),%rbx
    307d:	lea    (%r11,%rbx,1),%rbp
    3081:	add    %r9,%rbx
    3084:	mov    $0xc0,%ecx
    3089:	nopl   0x0(%rax)
    3090:	mov    %rbx,%r13
    3093:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    309b:	vmovss %xmm18,(%r15,%r12,4)
    30a2:	inc    %r12
    30a5:	add    $0x300,%rcx
    30ac:	add    $0xc00,%rbx
    30b3:	cmp    $0x9c0,%rcx
    30ba:	jne    3090 <main_graph_model+0xce0>
    30bc:	test   %r8b,%r8b
    30bf:	jne    2e70 <main_graph_model+0xac0>
    30c5:	data16 cs nopw 0x0(%rax,%rax,1)
    30d0:	vmovss -0x3000(%r13),%xmm18
    30da:	vmovss %xmm18,(%r15,%r12,4)
    30e1:	vmovss -0x2400(%r13),%xmm18
    30eb:	vmovss %xmm18,0x4(%r15,%r12,4)
    30f3:	vmovss -0x1800(%r13),%xmm18
    30fd:	vmovss %xmm18,0x8(%r15,%r12,4)
    3105:	vmovss -0xc00(%r13),%xmm18
    310f:	vmovss %xmm18,0xc(%r15,%r12,4)
    3117:	vmovss 0x0(%r13),%xmm18
    311e:	vmovss %xmm18,0x10(%r15,%r12,4)
    3126:	vmovss 0xc00(%r13),%xmm18
    3130:	vmovss %xmm18,0x14(%r15,%r12,4)
    3138:	vmovss 0x1800(%r13),%xmm18
    3142:	vmovss %xmm18,0x18(%r15,%r12,4)
    314a:	vmovss 0x2400(%r13),%xmm18
    3154:	vmovss %xmm18,0x1c(%r15,%r12,4)
    315c:	add    $0x8,%r12
    3160:	add    $0x6000,%r13
    3167:	cmp    $0x93,%r12
    316e:	jne    30d0 <main_graph_model+0xd20>
    3174:	jmp    2e70 <main_graph_model+0xac0>
    3179:	lea    0x400(%r14),%rdi
    3180:	lea    0x2df00(%rax),%rdx
    3187:	lea    0x6dd00(%r14),%rcx
    318e:	cmp    %rcx,%rsi
    3191:	setae  %cl
    3194:	cmp    %rdx,%rdi
    3197:	setae  %r8b
    319b:	or     %cl,%r8b
    319e:	lea    0x4000(%r14),%r9
    31a5:	xor    %r10d,%r10d
    31a8:	mov    %r14,%r11
    31ab:	mov    %rsi,%r15
    31ae:	jmp    31cc <main_graph_model+0xe1c>
    31b0:	inc    %r10
    31b3:	add    $0x24c,%r15
    31ba:	add    $0x4,%r11
    31be:	add    $0x4,%r9
    31c2:	cmp    $0x40,%r10
    31c6:	je     34b9 <main_graph_model+0x1109>
    31cc:	test   %r8b,%r8b
    31cf:	je     33b0 <main_graph_model+0x1000>
    31d5:	lea    (%rdi,%r10,4),%r12
    31d9:	kxnorb %k0,%k0,%k1
    31dd:	vxorps %xmm18,%xmm18,%xmm18
    31e3:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    31ea:	kxnorb %k0,%k0,%k1
    31ee:	vxorps %xmm19,%xmm19,%xmm19
    31f4:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    31fb:	kxnorb %k0,%k0,%k1
    31ff:	vxorps %xmm20,%xmm20,%xmm20
    3205:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    320c:	kxnorb %k0,%k0,%k1
    3210:	vxorps %xmm21,%xmm21,%xmm21
    3216:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    321d:	imul   $0x24c,%r10,%r13
    3224:	vmovups %ymm18,(%rsi,%r13,1)
    322b:	vmovups %ymm19,0x20(%rsi,%r13,1)
    3233:	vmovups %ymm20,0x40(%rsi,%r13,1)
    323b:	vmovups %ymm21,0x60(%rsi,%r13,1)
    3243:	kxnorb %k0,%k0,%k1
    3247:	vxorps %xmm18,%xmm18,%xmm18
    324d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    3254:	kxnorb %k0,%k0,%k1
    3258:	vxorps %xmm19,%xmm19,%xmm19
    325e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    3265:	kxnorb %k0,%k0,%k1
    3269:	vxorps %xmm20,%xmm20,%xmm20
    326f:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    3276:	kxnorb %k0,%k0,%k1
    327a:	vxorps %xmm21,%xmm21,%xmm21
    3280:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    3287:	vmovups %ymm18,0x80(%rsi,%r13,1)
    328f:	vmovups %ymm19,0xa0(%rsi,%r13,1)
    3297:	vmovups %ymm20,0xc0(%rsi,%r13,1)
    329f:	vmovups %ymm21,0xe0(%rsi,%r13,1)
    32a7:	kxnorb %k0,%k0,%k1
    32ab:	vxorps %xmm18,%xmm18,%xmm18
    32b1:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    32b8:	kxnorb %k0,%k0,%k1
    32bc:	vxorps %xmm19,%xmm19,%xmm19
    32c2:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    32c9:	kxnorb %k0,%k0,%k1
    32cd:	vxorps %xmm20,%xmm20,%xmm20
    32d3:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    32da:	kxnorb %k0,%k0,%k1
    32de:	vxorps %xmm21,%xmm21,%xmm21
    32e4:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    32eb:	vmovups %ymm18,0x100(%rsi,%r13,1)
    32f3:	vmovups %ymm19,0x120(%rsi,%r13,1)
    32fb:	vmovups %ymm20,0x140(%rsi,%r13,1)
    3303:	vmovups %ymm21,0x160(%rsi,%r13,1)
    330b:	kxnorb %k0,%k0,%k1
    330f:	vxorps %xmm18,%xmm18,%xmm18
    3315:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    331c:	kxnorb %k0,%k0,%k1
    3320:	vxorps %xmm19,%xmm19,%xmm19
    3326:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    332d:	kxnorb %k0,%k0,%k1
    3331:	vxorps %xmm20,%xmm20,%xmm20
    3337:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    333e:	kxnorb %k0,%k0,%k1
    3342:	vxorps %xmm21,%xmm21,%xmm21
    3348:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    334f:	vmovups %ymm18,0x180(%rsi,%r13,1)
    3357:	vmovups %ymm19,0x1a0(%rsi,%r13,1)
    335f:	vmovups %ymm20,0x1c0(%rsi,%r13,1)
    3367:	vmovups %ymm21,0x1e0(%rsi,%r13,1)
    336f:	kxnorb %k0,%k0,%k1
    3373:	vxorps %xmm18,%xmm18,%xmm18
    3379:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    3380:	vmovups %ymm18,0x200(%rsi,%r13,1)
    3388:	kxnorb %k0,%k0,%k1
    338c:	vxorps %xmm18,%xmm18,%xmm18
    3392:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    3399:	vmovups %ymm18,0x220(%rsi,%r13,1)
    33a1:	mov    $0x90,%r12d
    33a7:	jmp    33b3 <main_graph_model+0x1003>
    33a9:	nopl   0x0(%rax)
    33b0:	xor    %r12d,%r12d
    33b3:	mov    %r12d,%ecx
    33b6:	shl    $0xa,%ecx
    33b9:	lea    (%rcx,%rcx,2),%rbx
    33bd:	lea    (%r11,%rbx,1),%rbp
    33c1:	add    %r9,%rbx
    33c4:	mov    $0x100,%ecx
    33c9:	nopl   0x0(%rax)
    33d0:	mov    %rbx,%r13
    33d3:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    33db:	vmovss %xmm18,(%r15,%r12,4)
    33e2:	inc    %r12
    33e5:	add    $0x300,%rcx
    33ec:	add    $0xc00,%rbx
    33f3:	cmp    $0xa00,%rcx
    33fa:	jne    33d0 <main_graph_model+0x1020>
    33fc:	test   %r8b,%r8b
    33ff:	jne    31b0 <main_graph_model+0xe00>
    3405:	data16 cs nopw 0x0(%rax,%rax,1)
    3410:	vmovss -0x3000(%r13),%xmm18
    341a:	vmovss %xmm18,(%r15,%r12,4)
    3421:	vmovss -0x2400(%r13),%xmm18
    342b:	vmovss %xmm18,0x4(%r15,%r12,4)
    3433:	vmovss -0x1800(%r13),%xmm18
    343d:	vmovss %xmm18,0x8(%r15,%r12,4)
    3445:	vmovss -0xc00(%r13),%xmm18
    344f:	vmovss %xmm18,0xc(%r15,%r12,4)
    3457:	vmovss 0x0(%r13),%xmm18
    345e:	vmovss %xmm18,0x10(%r15,%r12,4)
    3466:	vmovss 0xc00(%r13),%xmm18
    3470:	vmovss %xmm18,0x14(%r15,%r12,4)
    3478:	vmovss 0x1800(%r13),%xmm18
    3482:	vmovss %xmm18,0x18(%r15,%r12,4)
    348a:	vmovss 0x2400(%r13),%xmm18
    3494:	vmovss %xmm18,0x1c(%r15,%r12,4)
    349c:	add    $0x8,%r12
    34a0:	add    $0x6000,%r13
    34a7:	cmp    $0x93,%r12
    34ae:	jne    3410 <main_graph_model+0x1060>
    34b4:	jmp    31b0 <main_graph_model+0xe00>
    34b9:	lea    0x500(%r14),%rdi
    34c0:	lea    0x37200(%rax),%rsi
    34c7:	lea    0x6de00(%r14),%rcx
    34ce:	cmp    %rcx,%rdx
    34d1:	setae  %cl
    34d4:	cmp    %rsi,%rdi
    34d7:	setae  %r8b
    34db:	or     %cl,%r8b
    34de:	lea    0x4100(%r14),%r9
    34e5:	xor    %r10d,%r10d
    34e8:	mov    %r14,%r11
    34eb:	mov    %rdx,%r15
    34ee:	jmp    350c <main_graph_model+0x115c>
    34f0:	inc    %r10
    34f3:	add    $0x24c,%r15
    34fa:	add    $0x4,%r11
    34fe:	add    $0x4,%r9
    3502:	cmp    $0x40,%r10
    3506:	je     37f9 <main_graph_model+0x1449>
    350c:	test   %r8b,%r8b
    350f:	je     36f0 <main_graph_model+0x1340>
    3515:	lea    (%rdi,%r10,4),%r12
    3519:	kxnorb %k0,%k0,%k1
    351d:	vxorps %xmm18,%xmm18,%xmm18
    3523:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    352a:	kxnorb %k0,%k0,%k1
    352e:	vxorps %xmm19,%xmm19,%xmm19
    3534:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    353b:	kxnorb %k0,%k0,%k1
    353f:	vxorps %xmm20,%xmm20,%xmm20
    3545:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    354c:	kxnorb %k0,%k0,%k1
    3550:	vxorps %xmm21,%xmm21,%xmm21
    3556:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    355d:	imul   $0x24c,%r10,%r13
    3564:	vmovups %ymm18,(%rdx,%r13,1)
    356b:	vmovups %ymm19,0x20(%rdx,%r13,1)
    3573:	vmovups %ymm20,0x40(%rdx,%r13,1)
    357b:	vmovups %ymm21,0x60(%rdx,%r13,1)
    3583:	kxnorb %k0,%k0,%k1
    3587:	vxorps %xmm18,%xmm18,%xmm18
    358d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    3594:	kxnorb %k0,%k0,%k1
    3598:	vxorps %xmm19,%xmm19,%xmm19
    359e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    35a5:	kxnorb %k0,%k0,%k1
    35a9:	vxorps %xmm20,%xmm20,%xmm20
    35af:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    35b6:	kxnorb %k0,%k0,%k1
    35ba:	vxorps %xmm21,%xmm21,%xmm21
    35c0:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    35c7:	vmovups %ymm18,0x80(%rdx,%r13,1)
    35cf:	vmovups %ymm19,0xa0(%rdx,%r13,1)
    35d7:	vmovups %ymm20,0xc0(%rdx,%r13,1)
    35df:	vmovups %ymm21,0xe0(%rdx,%r13,1)
    35e7:	kxnorb %k0,%k0,%k1
    35eb:	vxorps %xmm18,%xmm18,%xmm18
    35f1:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    35f8:	kxnorb %k0,%k0,%k1
    35fc:	vxorps %xmm19,%xmm19,%xmm19
    3602:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    3609:	kxnorb %k0,%k0,%k1
    360d:	vxorps %xmm20,%xmm20,%xmm20
    3613:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    361a:	kxnorb %k0,%k0,%k1
    361e:	vxorps %xmm21,%xmm21,%xmm21
    3624:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    362b:	vmovups %ymm18,0x100(%rdx,%r13,1)
    3633:	vmovups %ymm19,0x120(%rdx,%r13,1)
    363b:	vmovups %ymm20,0x140(%rdx,%r13,1)
    3643:	vmovups %ymm21,0x160(%rdx,%r13,1)
    364b:	kxnorb %k0,%k0,%k1
    364f:	vxorps %xmm18,%xmm18,%xmm18
    3655:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    365c:	kxnorb %k0,%k0,%k1
    3660:	vxorps %xmm19,%xmm19,%xmm19
    3666:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    366d:	kxnorb %k0,%k0,%k1
    3671:	vxorps %xmm20,%xmm20,%xmm20
    3677:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    367e:	kxnorb %k0,%k0,%k1
    3682:	vxorps %xmm21,%xmm21,%xmm21
    3688:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    368f:	vmovups %ymm18,0x180(%rdx,%r13,1)
    3697:	vmovups %ymm19,0x1a0(%rdx,%r13,1)
    369f:	vmovups %ymm20,0x1c0(%rdx,%r13,1)
    36a7:	vmovups %ymm21,0x1e0(%rdx,%r13,1)
    36af:	kxnorb %k0,%k0,%k1
    36b3:	vxorps %xmm18,%xmm18,%xmm18
    36b9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    36c0:	vmovups %ymm18,0x200(%rdx,%r13,1)
    36c8:	kxnorb %k0,%k0,%k1
    36cc:	vxorps %xmm18,%xmm18,%xmm18
    36d2:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    36d9:	vmovups %ymm18,0x220(%rdx,%r13,1)
    36e1:	mov    $0x90,%r12d
    36e7:	jmp    36f3 <main_graph_model+0x1343>
    36e9:	nopl   0x0(%rax)
    36f0:	xor    %r12d,%r12d
    36f3:	mov    %r12d,%ecx
    36f6:	shl    $0xa,%ecx
    36f9:	lea    (%rcx,%rcx,2),%rbx
    36fd:	lea    (%r11,%rbx,1),%rbp
    3701:	add    %r9,%rbx
    3704:	mov    $0x140,%ecx
    3709:	nopl   0x0(%rax)
    3710:	mov    %rbx,%r13
    3713:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    371b:	vmovss %xmm18,(%r15,%r12,4)
    3722:	inc    %r12
    3725:	add    $0x300,%rcx
    372c:	add    $0xc00,%rbx
    3733:	cmp    $0xa40,%rcx
    373a:	jne    3710 <main_graph_model+0x1360>
    373c:	test   %r8b,%r8b
    373f:	jne    34f0 <main_graph_model+0x1140>
    3745:	data16 cs nopw 0x0(%rax,%rax,1)
    3750:	vmovss -0x3000(%r13),%xmm18
    375a:	vmovss %xmm18,(%r15,%r12,4)
    3761:	vmovss -0x2400(%r13),%xmm18
    376b:	vmovss %xmm18,0x4(%r15,%r12,4)
    3773:	vmovss -0x1800(%r13),%xmm18
    377d:	vmovss %xmm18,0x8(%r15,%r12,4)
    3785:	vmovss -0xc00(%r13),%xmm18
    378f:	vmovss %xmm18,0xc(%r15,%r12,4)
    3797:	vmovss 0x0(%r13),%xmm18
    379e:	vmovss %xmm18,0x10(%r15,%r12,4)
    37a6:	vmovss 0xc00(%r13),%xmm18
    37b0:	vmovss %xmm18,0x14(%r15,%r12,4)
    37b8:	vmovss 0x1800(%r13),%xmm18
    37c2:	vmovss %xmm18,0x18(%r15,%r12,4)
    37ca:	vmovss 0x2400(%r13),%xmm18
    37d4:	vmovss %xmm18,0x1c(%r15,%r12,4)
    37dc:	add    $0x8,%r12
    37e0:	add    $0x6000,%r13
    37e7:	cmp    $0x93,%r12
    37ee:	jne    3750 <main_graph_model+0x13a0>
    37f4:	jmp    34f0 <main_graph_model+0x1140>
    37f9:	lea    0x600(%r14),%rdi
    3800:	lea    0x40500(%rax),%rdx
    3807:	lea    0x6df00(%r14),%rcx
    380e:	cmp    %rcx,%rsi
    3811:	setae  %cl
    3814:	cmp    %rdx,%rdi
    3817:	setae  %r8b
    381b:	or     %cl,%r8b
    381e:	lea    0x4200(%r14),%r9
    3825:	xor    %r10d,%r10d
    3828:	mov    %r14,%r11
    382b:	mov    %rsi,%r15
    382e:	jmp    384c <main_graph_model+0x149c>
    3830:	inc    %r10
    3833:	add    $0x24c,%r15
    383a:	add    $0x4,%r11
    383e:	add    $0x4,%r9
    3842:	cmp    $0x40,%r10
    3846:	je     3b39 <main_graph_model+0x1789>
    384c:	test   %r8b,%r8b
    384f:	je     3a30 <main_graph_model+0x1680>
    3855:	lea    (%rdi,%r10,4),%r12
    3859:	kxnorb %k0,%k0,%k1
    385d:	vxorps %xmm18,%xmm18,%xmm18
    3863:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    386a:	kxnorb %k0,%k0,%k1
    386e:	vxorps %xmm19,%xmm19,%xmm19
    3874:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    387b:	kxnorb %k0,%k0,%k1
    387f:	vxorps %xmm20,%xmm20,%xmm20
    3885:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    388c:	kxnorb %k0,%k0,%k1
    3890:	vxorps %xmm21,%xmm21,%xmm21
    3896:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    389d:	imul   $0x24c,%r10,%r13
    38a4:	vmovups %ymm18,(%rsi,%r13,1)
    38ab:	vmovups %ymm19,0x20(%rsi,%r13,1)
    38b3:	vmovups %ymm20,0x40(%rsi,%r13,1)
    38bb:	vmovups %ymm21,0x60(%rsi,%r13,1)
    38c3:	kxnorb %k0,%k0,%k1
    38c7:	vxorps %xmm18,%xmm18,%xmm18
    38cd:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    38d4:	kxnorb %k0,%k0,%k1
    38d8:	vxorps %xmm19,%xmm19,%xmm19
    38de:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    38e5:	kxnorb %k0,%k0,%k1
    38e9:	vxorps %xmm20,%xmm20,%xmm20
    38ef:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    38f6:	kxnorb %k0,%k0,%k1
    38fa:	vxorps %xmm21,%xmm21,%xmm21
    3900:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    3907:	vmovups %ymm18,0x80(%rsi,%r13,1)
    390f:	vmovups %ymm19,0xa0(%rsi,%r13,1)
    3917:	vmovups %ymm20,0xc0(%rsi,%r13,1)
    391f:	vmovups %ymm21,0xe0(%rsi,%r13,1)
    3927:	kxnorb %k0,%k0,%k1
    392b:	vxorps %xmm18,%xmm18,%xmm18
    3931:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    3938:	kxnorb %k0,%k0,%k1
    393c:	vxorps %xmm19,%xmm19,%xmm19
    3942:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    3949:	kxnorb %k0,%k0,%k1
    394d:	vxorps %xmm20,%xmm20,%xmm20
    3953:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    395a:	kxnorb %k0,%k0,%k1
    395e:	vxorps %xmm21,%xmm21,%xmm21
    3964:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    396b:	vmovups %ymm18,0x100(%rsi,%r13,1)
    3973:	vmovups %ymm19,0x120(%rsi,%r13,1)
    397b:	vmovups %ymm20,0x140(%rsi,%r13,1)
    3983:	vmovups %ymm21,0x160(%rsi,%r13,1)
    398b:	kxnorb %k0,%k0,%k1
    398f:	vxorps %xmm18,%xmm18,%xmm18
    3995:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    399c:	kxnorb %k0,%k0,%k1
    39a0:	vxorps %xmm19,%xmm19,%xmm19
    39a6:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    39ad:	kxnorb %k0,%k0,%k1
    39b1:	vxorps %xmm20,%xmm20,%xmm20
    39b7:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    39be:	kxnorb %k0,%k0,%k1
    39c2:	vxorps %xmm21,%xmm21,%xmm21
    39c8:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    39cf:	vmovups %ymm18,0x180(%rsi,%r13,1)
    39d7:	vmovups %ymm19,0x1a0(%rsi,%r13,1)
    39df:	vmovups %ymm20,0x1c0(%rsi,%r13,1)
    39e7:	vmovups %ymm21,0x1e0(%rsi,%r13,1)
    39ef:	kxnorb %k0,%k0,%k1
    39f3:	vxorps %xmm18,%xmm18,%xmm18
    39f9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    3a00:	vmovups %ymm18,0x200(%rsi,%r13,1)
    3a08:	kxnorb %k0,%k0,%k1
    3a0c:	vxorps %xmm18,%xmm18,%xmm18
    3a12:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    3a19:	vmovups %ymm18,0x220(%rsi,%r13,1)
    3a21:	mov    $0x90,%r12d
    3a27:	jmp    3a33 <main_graph_model+0x1683>
    3a29:	nopl   0x0(%rax)
    3a30:	xor    %r12d,%r12d
    3a33:	mov    %r12d,%ecx
    3a36:	shl    $0xa,%ecx
    3a39:	lea    (%rcx,%rcx,2),%rbx
    3a3d:	lea    (%r11,%rbx,1),%rbp
    3a41:	add    %r9,%rbx
    3a44:	mov    $0x180,%ecx
    3a49:	nopl   0x0(%rax)
    3a50:	mov    %rbx,%r13
    3a53:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    3a5b:	vmovss %xmm18,(%r15,%r12,4)
    3a62:	inc    %r12
    3a65:	add    $0x300,%rcx
    3a6c:	add    $0xc00,%rbx
    3a73:	cmp    $0xa80,%rcx
    3a7a:	jne    3a50 <main_graph_model+0x16a0>
    3a7c:	test   %r8b,%r8b
    3a7f:	jne    3830 <main_graph_model+0x1480>
    3a85:	data16 cs nopw 0x0(%rax,%rax,1)
    3a90:	vmovss -0x3000(%r13),%xmm18
    3a9a:	vmovss %xmm18,(%r15,%r12,4)
    3aa1:	vmovss -0x2400(%r13),%xmm18
    3aab:	vmovss %xmm18,0x4(%r15,%r12,4)
    3ab3:	vmovss -0x1800(%r13),%xmm18
    3abd:	vmovss %xmm18,0x8(%r15,%r12,4)
    3ac5:	vmovss -0xc00(%r13),%xmm18
    3acf:	vmovss %xmm18,0xc(%r15,%r12,4)
    3ad7:	vmovss 0x0(%r13),%xmm18
    3ade:	vmovss %xmm18,0x10(%r15,%r12,4)
    3ae6:	vmovss 0xc00(%r13),%xmm18
    3af0:	vmovss %xmm18,0x14(%r15,%r12,4)
    3af8:	vmovss 0x1800(%r13),%xmm18
    3b02:	vmovss %xmm18,0x18(%r15,%r12,4)
    3b0a:	vmovss 0x2400(%r13),%xmm18
    3b14:	vmovss %xmm18,0x1c(%r15,%r12,4)
    3b1c:	add    $0x8,%r12
    3b20:	add    $0x6000,%r13
    3b27:	cmp    $0x93,%r12
    3b2e:	jne    3a90 <main_graph_model+0x16e0>
    3b34:	jmp    3830 <main_graph_model+0x1480>
    3b39:	lea    0x700(%r14),%rdi
    3b40:	lea    0x49800(%rax),%rsi
    3b47:	lea    0x6e000(%r14),%rcx
    3b4e:	cmp    %rcx,%rdx
    3b51:	setae  %cl
    3b54:	cmp    %rsi,%rdi
    3b57:	setae  %r8b
    3b5b:	or     %cl,%r8b
    3b5e:	lea    0x4300(%r14),%r9
    3b65:	xor    %r10d,%r10d
    3b68:	mov    %r14,%r11
    3b6b:	mov    %rdx,%r15
    3b6e:	jmp    3b8c <main_graph_model+0x17dc>
    3b70:	inc    %r10
    3b73:	add    $0x24c,%r15
    3b7a:	add    $0x4,%r11
    3b7e:	add    $0x4,%r9
    3b82:	cmp    $0x40,%r10
    3b86:	je     3e79 <main_graph_model+0x1ac9>
    3b8c:	test   %r8b,%r8b
    3b8f:	je     3d70 <main_graph_model+0x19c0>
    3b95:	lea    (%rdi,%r10,4),%r12
    3b99:	kxnorb %k0,%k0,%k1
    3b9d:	vxorps %xmm18,%xmm18,%xmm18
    3ba3:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    3baa:	kxnorb %k0,%k0,%k1
    3bae:	vxorps %xmm19,%xmm19,%xmm19
    3bb4:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    3bbb:	kxnorb %k0,%k0,%k1
    3bbf:	vxorps %xmm20,%xmm20,%xmm20
    3bc5:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    3bcc:	kxnorb %k0,%k0,%k1
    3bd0:	vxorps %xmm21,%xmm21,%xmm21
    3bd6:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    3bdd:	imul   $0x24c,%r10,%r13
    3be4:	vmovups %ymm18,(%rdx,%r13,1)
    3beb:	vmovups %ymm19,0x20(%rdx,%r13,1)
    3bf3:	vmovups %ymm20,0x40(%rdx,%r13,1)
    3bfb:	vmovups %ymm21,0x60(%rdx,%r13,1)
    3c03:	kxnorb %k0,%k0,%k1
    3c07:	vxorps %xmm18,%xmm18,%xmm18
    3c0d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    3c14:	kxnorb %k0,%k0,%k1
    3c18:	vxorps %xmm19,%xmm19,%xmm19
    3c1e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    3c25:	kxnorb %k0,%k0,%k1
    3c29:	vxorps %xmm20,%xmm20,%xmm20
    3c2f:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    3c36:	kxnorb %k0,%k0,%k1
    3c3a:	vxorps %xmm21,%xmm21,%xmm21
    3c40:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    3c47:	vmovups %ymm18,0x80(%rdx,%r13,1)
    3c4f:	vmovups %ymm19,0xa0(%rdx,%r13,1)
    3c57:	vmovups %ymm20,0xc0(%rdx,%r13,1)
    3c5f:	vmovups %ymm21,0xe0(%rdx,%r13,1)
    3c67:	kxnorb %k0,%k0,%k1
    3c6b:	vxorps %xmm18,%xmm18,%xmm18
    3c71:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    3c78:	kxnorb %k0,%k0,%k1
    3c7c:	vxorps %xmm19,%xmm19,%xmm19
    3c82:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    3c89:	kxnorb %k0,%k0,%k1
    3c8d:	vxorps %xmm20,%xmm20,%xmm20
    3c93:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    3c9a:	kxnorb %k0,%k0,%k1
    3c9e:	vxorps %xmm21,%xmm21,%xmm21
    3ca4:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    3cab:	vmovups %ymm18,0x100(%rdx,%r13,1)
    3cb3:	vmovups %ymm19,0x120(%rdx,%r13,1)
    3cbb:	vmovups %ymm20,0x140(%rdx,%r13,1)
    3cc3:	vmovups %ymm21,0x160(%rdx,%r13,1)
    3ccb:	kxnorb %k0,%k0,%k1
    3ccf:	vxorps %xmm18,%xmm18,%xmm18
    3cd5:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    3cdc:	kxnorb %k0,%k0,%k1
    3ce0:	vxorps %xmm19,%xmm19,%xmm19
    3ce6:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    3ced:	kxnorb %k0,%k0,%k1
    3cf1:	vxorps %xmm20,%xmm20,%xmm20
    3cf7:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    3cfe:	kxnorb %k0,%k0,%k1
    3d02:	vxorps %xmm21,%xmm21,%xmm21
    3d08:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    3d0f:	vmovups %ymm18,0x180(%rdx,%r13,1)
    3d17:	vmovups %ymm19,0x1a0(%rdx,%r13,1)
    3d1f:	vmovups %ymm20,0x1c0(%rdx,%r13,1)
    3d27:	vmovups %ymm21,0x1e0(%rdx,%r13,1)
    3d2f:	kxnorb %k0,%k0,%k1
    3d33:	vxorps %xmm18,%xmm18,%xmm18
    3d39:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    3d40:	vmovups %ymm18,0x200(%rdx,%r13,1)
    3d48:	kxnorb %k0,%k0,%k1
    3d4c:	vxorps %xmm18,%xmm18,%xmm18
    3d52:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    3d59:	vmovups %ymm18,0x220(%rdx,%r13,1)
    3d61:	mov    $0x90,%r12d
    3d67:	jmp    3d73 <main_graph_model+0x19c3>
    3d69:	nopl   0x0(%rax)
    3d70:	xor    %r12d,%r12d
    3d73:	mov    %r12d,%ecx
    3d76:	shl    $0xa,%ecx
    3d79:	lea    (%rcx,%rcx,2),%rbx
    3d7d:	lea    (%r11,%rbx,1),%rbp
    3d81:	add    %r9,%rbx
    3d84:	mov    $0x1c0,%ecx
    3d89:	nopl   0x0(%rax)
    3d90:	mov    %rbx,%r13
    3d93:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    3d9b:	vmovss %xmm18,(%r15,%r12,4)
    3da2:	inc    %r12
    3da5:	add    $0x300,%rcx
    3dac:	add    $0xc00,%rbx
    3db3:	cmp    $0xac0,%rcx
    3dba:	jne    3d90 <main_graph_model+0x19e0>
    3dbc:	test   %r8b,%r8b
    3dbf:	jne    3b70 <main_graph_model+0x17c0>
    3dc5:	data16 cs nopw 0x0(%rax,%rax,1)
    3dd0:	vmovss -0x3000(%r13),%xmm18
    3dda:	vmovss %xmm18,(%r15,%r12,4)
    3de1:	vmovss -0x2400(%r13),%xmm18
    3deb:	vmovss %xmm18,0x4(%r15,%r12,4)
    3df3:	vmovss -0x1800(%r13),%xmm18
    3dfd:	vmovss %xmm18,0x8(%r15,%r12,4)
    3e05:	vmovss -0xc00(%r13),%xmm18
    3e0f:	vmovss %xmm18,0xc(%r15,%r12,4)
    3e17:	vmovss 0x0(%r13),%xmm18
    3e1e:	vmovss %xmm18,0x10(%r15,%r12,4)
    3e26:	vmovss 0xc00(%r13),%xmm18
    3e30:	vmovss %xmm18,0x14(%r15,%r12,4)
    3e38:	vmovss 0x1800(%r13),%xmm18
    3e42:	vmovss %xmm18,0x18(%r15,%r12,4)
    3e4a:	vmovss 0x2400(%r13),%xmm18
    3e54:	vmovss %xmm18,0x1c(%r15,%r12,4)
    3e5c:	add    $0x8,%r12
    3e60:	add    $0x6000,%r13
    3e67:	cmp    $0x93,%r12
    3e6e:	jne    3dd0 <main_graph_model+0x1a20>
    3e74:	jmp    3b70 <main_graph_model+0x17c0>
    3e79:	lea    0x800(%r14),%rdi
    3e80:	lea    0x52b00(%rax),%rdx
    3e87:	lea    0x6e100(%r14),%rcx
    3e8e:	cmp    %rcx,%rsi
    3e91:	setae  %cl
    3e94:	cmp    %rdx,%rdi
    3e97:	setae  %r8b
    3e9b:	or     %cl,%r8b
    3e9e:	lea    0x4400(%r14),%r9
    3ea5:	xor    %r10d,%r10d
    3ea8:	mov    %r14,%r11
    3eab:	mov    %rsi,%r15
    3eae:	jmp    3ecc <main_graph_model+0x1b1c>
    3eb0:	inc    %r10
    3eb3:	add    $0x24c,%r15
    3eba:	add    $0x4,%r11
    3ebe:	add    $0x4,%r9
    3ec2:	cmp    $0x40,%r10
    3ec6:	je     41b9 <main_graph_model+0x1e09>
    3ecc:	test   %r8b,%r8b
    3ecf:	je     40b0 <main_graph_model+0x1d00>
    3ed5:	lea    (%rdi,%r10,4),%r12
    3ed9:	kxnorb %k0,%k0,%k1
    3edd:	vxorps %xmm18,%xmm18,%xmm18
    3ee3:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    3eea:	kxnorb %k0,%k0,%k1
    3eee:	vxorps %xmm19,%xmm19,%xmm19
    3ef4:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    3efb:	kxnorb %k0,%k0,%k1
    3eff:	vxorps %xmm20,%xmm20,%xmm20
    3f05:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    3f0c:	kxnorb %k0,%k0,%k1
    3f10:	vxorps %xmm21,%xmm21,%xmm21
    3f16:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    3f1d:	imul   $0x24c,%r10,%r13
    3f24:	vmovups %ymm18,(%rsi,%r13,1)
    3f2b:	vmovups %ymm19,0x20(%rsi,%r13,1)
    3f33:	vmovups %ymm20,0x40(%rsi,%r13,1)
    3f3b:	vmovups %ymm21,0x60(%rsi,%r13,1)
    3f43:	kxnorb %k0,%k0,%k1
    3f47:	vxorps %xmm18,%xmm18,%xmm18
    3f4d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    3f54:	kxnorb %k0,%k0,%k1
    3f58:	vxorps %xmm19,%xmm19,%xmm19
    3f5e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    3f65:	kxnorb %k0,%k0,%k1
    3f69:	vxorps %xmm20,%xmm20,%xmm20
    3f6f:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    3f76:	kxnorb %k0,%k0,%k1
    3f7a:	vxorps %xmm21,%xmm21,%xmm21
    3f80:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    3f87:	vmovups %ymm18,0x80(%rsi,%r13,1)
    3f8f:	vmovups %ymm19,0xa0(%rsi,%r13,1)
    3f97:	vmovups %ymm20,0xc0(%rsi,%r13,1)
    3f9f:	vmovups %ymm21,0xe0(%rsi,%r13,1)
    3fa7:	kxnorb %k0,%k0,%k1
    3fab:	vxorps %xmm18,%xmm18,%xmm18
    3fb1:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    3fb8:	kxnorb %k0,%k0,%k1
    3fbc:	vxorps %xmm19,%xmm19,%xmm19
    3fc2:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    3fc9:	kxnorb %k0,%k0,%k1
    3fcd:	vxorps %xmm20,%xmm20,%xmm20
    3fd3:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    3fda:	kxnorb %k0,%k0,%k1
    3fde:	vxorps %xmm21,%xmm21,%xmm21
    3fe4:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    3feb:	vmovups %ymm18,0x100(%rsi,%r13,1)
    3ff3:	vmovups %ymm19,0x120(%rsi,%r13,1)
    3ffb:	vmovups %ymm20,0x140(%rsi,%r13,1)
    4003:	vmovups %ymm21,0x160(%rsi,%r13,1)
    400b:	kxnorb %k0,%k0,%k1
    400f:	vxorps %xmm18,%xmm18,%xmm18
    4015:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    401c:	kxnorb %k0,%k0,%k1
    4020:	vxorps %xmm19,%xmm19,%xmm19
    4026:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    402d:	kxnorb %k0,%k0,%k1
    4031:	vxorps %xmm20,%xmm20,%xmm20
    4037:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    403e:	kxnorb %k0,%k0,%k1
    4042:	vxorps %xmm21,%xmm21,%xmm21
    4048:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    404f:	vmovups %ymm18,0x180(%rsi,%r13,1)
    4057:	vmovups %ymm19,0x1a0(%rsi,%r13,1)
    405f:	vmovups %ymm20,0x1c0(%rsi,%r13,1)
    4067:	vmovups %ymm21,0x1e0(%rsi,%r13,1)
    406f:	kxnorb %k0,%k0,%k1
    4073:	vxorps %xmm18,%xmm18,%xmm18
    4079:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    4080:	vmovups %ymm18,0x200(%rsi,%r13,1)
    4088:	kxnorb %k0,%k0,%k1
    408c:	vxorps %xmm18,%xmm18,%xmm18
    4092:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    4099:	vmovups %ymm18,0x220(%rsi,%r13,1)
    40a1:	mov    $0x90,%r12d
    40a7:	jmp    40b3 <main_graph_model+0x1d03>
    40a9:	nopl   0x0(%rax)
    40b0:	xor    %r12d,%r12d
    40b3:	mov    %r12d,%ecx
    40b6:	shl    $0xa,%ecx
    40b9:	lea    (%rcx,%rcx,2),%rbx
    40bd:	lea    (%r11,%rbx,1),%rbp
    40c1:	add    %r9,%rbx
    40c4:	mov    $0x200,%ecx
    40c9:	nopl   0x0(%rax)
    40d0:	mov    %rbx,%r13
    40d3:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    40db:	vmovss %xmm18,(%r15,%r12,4)
    40e2:	inc    %r12
    40e5:	add    $0x300,%rcx
    40ec:	add    $0xc00,%rbx
    40f3:	cmp    $0xb00,%rcx
    40fa:	jne    40d0 <main_graph_model+0x1d20>
    40fc:	test   %r8b,%r8b
    40ff:	jne    3eb0 <main_graph_model+0x1b00>
    4105:	data16 cs nopw 0x0(%rax,%rax,1)
    4110:	vmovss -0x3000(%r13),%xmm18
    411a:	vmovss %xmm18,(%r15,%r12,4)
    4121:	vmovss -0x2400(%r13),%xmm18
    412b:	vmovss %xmm18,0x4(%r15,%r12,4)
    4133:	vmovss -0x1800(%r13),%xmm18
    413d:	vmovss %xmm18,0x8(%r15,%r12,4)
    4145:	vmovss -0xc00(%r13),%xmm18
    414f:	vmovss %xmm18,0xc(%r15,%r12,4)
    4157:	vmovss 0x0(%r13),%xmm18
    415e:	vmovss %xmm18,0x10(%r15,%r12,4)
    4166:	vmovss 0xc00(%r13),%xmm18
    4170:	vmovss %xmm18,0x14(%r15,%r12,4)
    4178:	vmovss 0x1800(%r13),%xmm18
    4182:	vmovss %xmm18,0x18(%r15,%r12,4)
    418a:	vmovss 0x2400(%r13),%xmm18
    4194:	vmovss %xmm18,0x1c(%r15,%r12,4)
    419c:	add    $0x8,%r12
    41a0:	add    $0x6000,%r13
    41a7:	cmp    $0x93,%r12
    41ae:	jne    4110 <main_graph_model+0x1d60>
    41b4:	jmp    3eb0 <main_graph_model+0x1b00>
    41b9:	lea    0x900(%r14),%rdi
    41c0:	lea    0x5be00(%rax),%rsi
    41c7:	lea    0x6e200(%r14),%rcx
    41ce:	cmp    %rcx,%rdx
    41d1:	setae  %cl
    41d4:	cmp    %rsi,%rdi
    41d7:	setae  %r8b
    41db:	or     %cl,%r8b
    41de:	lea    0x4500(%r14),%r9
    41e5:	xor    %r10d,%r10d
    41e8:	mov    %r14,%r11
    41eb:	mov    %rdx,%r15
    41ee:	jmp    420c <main_graph_model+0x1e5c>
    41f0:	inc    %r10
    41f3:	add    $0x24c,%r15
    41fa:	add    $0x4,%r11
    41fe:	add    $0x4,%r9
    4202:	cmp    $0x40,%r10
    4206:	je     44f9 <main_graph_model+0x2149>
    420c:	test   %r8b,%r8b
    420f:	je     43f0 <main_graph_model+0x2040>
    4215:	lea    (%rdi,%r10,4),%r12
    4219:	kxnorb %k0,%k0,%k1
    421d:	vxorps %xmm18,%xmm18,%xmm18
    4223:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    422a:	kxnorb %k0,%k0,%k1
    422e:	vxorps %xmm19,%xmm19,%xmm19
    4234:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    423b:	kxnorb %k0,%k0,%k1
    423f:	vxorps %xmm20,%xmm20,%xmm20
    4245:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    424c:	kxnorb %k0,%k0,%k1
    4250:	vxorps %xmm21,%xmm21,%xmm21
    4256:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    425d:	imul   $0x24c,%r10,%r13
    4264:	vmovups %ymm18,(%rdx,%r13,1)
    426b:	vmovups %ymm19,0x20(%rdx,%r13,1)
    4273:	vmovups %ymm20,0x40(%rdx,%r13,1)
    427b:	vmovups %ymm21,0x60(%rdx,%r13,1)
    4283:	kxnorb %k0,%k0,%k1
    4287:	vxorps %xmm18,%xmm18,%xmm18
    428d:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    4294:	kxnorb %k0,%k0,%k1
    4298:	vxorps %xmm19,%xmm19,%xmm19
    429e:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    42a5:	kxnorb %k0,%k0,%k1
    42a9:	vxorps %xmm20,%xmm20,%xmm20
    42af:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    42b6:	kxnorb %k0,%k0,%k1
    42ba:	vxorps %xmm21,%xmm21,%xmm21
    42c0:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    42c7:	vmovups %ymm18,0x80(%rdx,%r13,1)
    42cf:	vmovups %ymm19,0xa0(%rdx,%r13,1)
    42d7:	vmovups %ymm20,0xc0(%rdx,%r13,1)
    42df:	vmovups %ymm21,0xe0(%rdx,%r13,1)
    42e7:	kxnorb %k0,%k0,%k1
    42eb:	vxorps %xmm18,%xmm18,%xmm18
    42f1:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    42f8:	kxnorb %k0,%k0,%k1
    42fc:	vxorps %xmm19,%xmm19,%xmm19
    4302:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    4309:	kxnorb %k0,%k0,%k1
    430d:	vxorps %xmm20,%xmm20,%xmm20
    4313:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    431a:	kxnorb %k0,%k0,%k1
    431e:	vxorps %xmm21,%xmm21,%xmm21
    4324:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    432b:	vmovups %ymm18,0x100(%rdx,%r13,1)
    4333:	vmovups %ymm19,0x120(%rdx,%r13,1)
    433b:	vmovups %ymm20,0x140(%rdx,%r13,1)
    4343:	vmovups %ymm21,0x160(%rdx,%r13,1)
    434b:	kxnorb %k0,%k0,%k1
    434f:	vxorps %xmm18,%xmm18,%xmm18
    4355:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    435c:	kxnorb %k0,%k0,%k1
    4360:	vxorps %xmm19,%xmm19,%xmm19
    4366:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    436d:	kxnorb %k0,%k0,%k1
    4371:	vxorps %xmm20,%xmm20,%xmm20
    4377:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    437e:	kxnorb %k0,%k0,%k1
    4382:	vxorps %xmm21,%xmm21,%xmm21
    4388:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    438f:	vmovups %ymm18,0x180(%rdx,%r13,1)
    4397:	vmovups %ymm19,0x1a0(%rdx,%r13,1)
    439f:	vmovups %ymm20,0x1c0(%rdx,%r13,1)
    43a7:	vmovups %ymm21,0x1e0(%rdx,%r13,1)
    43af:	kxnorb %k0,%k0,%k1
    43b3:	vxorps %xmm18,%xmm18,%xmm18
    43b9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    43c0:	vmovups %ymm18,0x200(%rdx,%r13,1)
    43c8:	kxnorb %k0,%k0,%k1
    43cc:	vxorps %xmm18,%xmm18,%xmm18
    43d2:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    43d9:	vmovups %ymm18,0x220(%rdx,%r13,1)
    43e1:	mov    $0x90,%r12d
    43e7:	jmp    43f3 <main_graph_model+0x2043>
    43e9:	nopl   0x0(%rax)
    43f0:	xor    %r12d,%r12d
    43f3:	mov    %r12d,%ecx
    43f6:	shl    $0xa,%ecx
    43f9:	lea    (%rcx,%rcx,2),%rbx
    43fd:	lea    (%r11,%rbx,1),%rbp
    4401:	add    %r9,%rbx
    4404:	mov    $0x240,%ecx
    4409:	nopl   0x0(%rax)
    4410:	mov    %rbx,%r13
    4413:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    441b:	vmovss %xmm18,(%r15,%r12,4)
    4422:	inc    %r12
    4425:	add    $0x300,%rcx
    442c:	add    $0xc00,%rbx
    4433:	cmp    $0xb40,%rcx
    443a:	jne    4410 <main_graph_model+0x2060>
    443c:	test   %r8b,%r8b
    443f:	jne    41f0 <main_graph_model+0x1e40>
    4445:	data16 cs nopw 0x0(%rax,%rax,1)
    4450:	vmovss -0x3000(%r13),%xmm18
    445a:	vmovss %xmm18,(%r15,%r12,4)
    4461:	vmovss -0x2400(%r13),%xmm18
    446b:	vmovss %xmm18,0x4(%r15,%r12,4)
    4473:	vmovss -0x1800(%r13),%xmm18
    447d:	vmovss %xmm18,0x8(%r15,%r12,4)
    4485:	vmovss -0xc00(%r13),%xmm18
    448f:	vmovss %xmm18,0xc(%r15,%r12,4)
    4497:	vmovss 0x0(%r13),%xmm18
    449e:	vmovss %xmm18,0x10(%r15,%r12,4)
    44a6:	vmovss 0xc00(%r13),%xmm18
    44b0:	vmovss %xmm18,0x14(%r15,%r12,4)
    44b8:	vmovss 0x1800(%r13),%xmm18
    44c2:	vmovss %xmm18,0x18(%r15,%r12,4)
    44ca:	vmovss 0x2400(%r13),%xmm18
    44d4:	vmovss %xmm18,0x1c(%r15,%r12,4)
    44dc:	add    $0x8,%r12
    44e0:	add    $0x6000,%r13
    44e7:	cmp    $0x93,%r12
    44ee:	jne    4450 <main_graph_model+0x20a0>
    44f4:	jmp    41f0 <main_graph_model+0x1e40>
    44f9:	lea    0xa00(%r14),%rdi
    4500:	lea    0x65100(%rax),%rdx
    4507:	lea    0x6e300(%r14),%rcx
    450e:	cmp    %rcx,%rsi
    4511:	setae  %cl
    4514:	cmp    %rdx,%rdi
    4517:	setae  %r8b
    451b:	or     %cl,%r8b
    451e:	lea    0x4600(%r14),%r9
    4525:	xor    %r10d,%r10d
    4528:	mov    %r14,%r11
    452b:	mov    %rsi,%r15
    452e:	jmp    454c <main_graph_model+0x219c>
    4530:	inc    %r10
    4533:	add    $0x24c,%r15
    453a:	add    $0x4,%r11
    453e:	add    $0x4,%r9
    4542:	cmp    $0x40,%r10
    4546:	je     4839 <main_graph_model+0x2489>
    454c:	test   %r8b,%r8b
    454f:	je     4730 <main_graph_model+0x2380>
    4555:	lea    (%rdi,%r10,4),%r12
    4559:	kxnorb %k0,%k0,%k1
    455d:	vxorps %xmm18,%xmm18,%xmm18
    4563:	vgatherdps (%r12,%ymm0,1),%ymm18{%k1}
    456a:	kxnorb %k0,%k0,%k1
    456e:	vxorps %xmm19,%xmm19,%xmm19
    4574:	vgatherdps (%r12,%ymm1,1),%ymm19{%k1}
    457b:	kxnorb %k0,%k0,%k1
    457f:	vxorps %xmm20,%xmm20,%xmm20
    4585:	vgatherdps (%r12,%ymm2,1),%ymm20{%k1}
    458c:	kxnorb %k0,%k0,%k1
    4590:	vxorps %xmm21,%xmm21,%xmm21
    4596:	vgatherdps (%r12,%ymm3,1),%ymm21{%k1}
    459d:	imul   $0x24c,%r10,%r13
    45a4:	vmovups %ymm18,(%rsi,%r13,1)
    45ab:	vmovups %ymm19,0x20(%rsi,%r13,1)
    45b3:	vmovups %ymm20,0x40(%rsi,%r13,1)
    45bb:	vmovups %ymm21,0x60(%rsi,%r13,1)
    45c3:	kxnorb %k0,%k0,%k1
    45c7:	vxorps %xmm18,%xmm18,%xmm18
    45cd:	vgatherdps (%r12,%ymm4,1),%ymm18{%k1}
    45d4:	kxnorb %k0,%k0,%k1
    45d8:	vxorps %xmm19,%xmm19,%xmm19
    45de:	vgatherdps (%r12,%ymm5,1),%ymm19{%k1}
    45e5:	kxnorb %k0,%k0,%k1
    45e9:	vxorps %xmm20,%xmm20,%xmm20
    45ef:	vgatherdps (%r12,%ymm6,1),%ymm20{%k1}
    45f6:	kxnorb %k0,%k0,%k1
    45fa:	vxorps %xmm21,%xmm21,%xmm21
    4600:	vgatherdps (%r12,%ymm7,1),%ymm21{%k1}
    4607:	vmovups %ymm18,0x80(%rsi,%r13,1)
    460f:	vmovups %ymm19,0xa0(%rsi,%r13,1)
    4617:	vmovups %ymm20,0xc0(%rsi,%r13,1)
    461f:	vmovups %ymm21,0xe0(%rsi,%r13,1)
    4627:	kxnorb %k0,%k0,%k1
    462b:	vxorps %xmm18,%xmm18,%xmm18
    4631:	vgatherdps (%r12,%ymm8,1),%ymm18{%k1}
    4638:	kxnorb %k0,%k0,%k1
    463c:	vxorps %xmm19,%xmm19,%xmm19
    4642:	vgatherdps (%r12,%ymm9,1),%ymm19{%k1}
    4649:	kxnorb %k0,%k0,%k1
    464d:	vxorps %xmm20,%xmm20,%xmm20
    4653:	vgatherdps (%r12,%ymm10,1),%ymm20{%k1}
    465a:	kxnorb %k0,%k0,%k1
    465e:	vxorps %xmm21,%xmm21,%xmm21
    4664:	vgatherdps (%r12,%ymm11,1),%ymm21{%k1}
    466b:	vmovups %ymm18,0x100(%rsi,%r13,1)
    4673:	vmovups %ymm19,0x120(%rsi,%r13,1)
    467b:	vmovups %ymm20,0x140(%rsi,%r13,1)
    4683:	vmovups %ymm21,0x160(%rsi,%r13,1)
    468b:	kxnorb %k0,%k0,%k1
    468f:	vxorps %xmm18,%xmm18,%xmm18
    4695:	vgatherdps (%r12,%ymm12,1),%ymm18{%k1}
    469c:	kxnorb %k0,%k0,%k1
    46a0:	vxorps %xmm19,%xmm19,%xmm19
    46a6:	vgatherdps (%r12,%ymm13,1),%ymm19{%k1}
    46ad:	kxnorb %k0,%k0,%k1
    46b1:	vxorps %xmm20,%xmm20,%xmm20
    46b7:	vgatherdps (%r12,%ymm14,1),%ymm20{%k1}
    46be:	kxnorb %k0,%k0,%k1
    46c2:	vxorps %xmm21,%xmm21,%xmm21
    46c8:	vgatherdps (%r12,%ymm15,1),%ymm21{%k1}
    46cf:	vmovups %ymm18,0x180(%rsi,%r13,1)
    46d7:	vmovups %ymm19,0x1a0(%rsi,%r13,1)
    46df:	vmovups %ymm20,0x1c0(%rsi,%r13,1)
    46e7:	vmovups %ymm21,0x1e0(%rsi,%r13,1)
    46ef:	kxnorb %k0,%k0,%k1
    46f3:	vxorps %xmm18,%xmm18,%xmm18
    46f9:	vgatherdps (%r12,%ymm16,1),%ymm18{%k1}
    4700:	vmovups %ymm18,0x200(%rsi,%r13,1)
    4708:	kxnorb %k0,%k0,%k1
    470c:	vxorps %xmm18,%xmm18,%xmm18
    4712:	vgatherdps (%r12,%ymm17,1),%ymm18{%k1}
    4719:	vmovups %ymm18,0x220(%rsi,%r13,1)
    4721:	mov    $0x90,%r12d
    4727:	jmp    4733 <main_graph_model+0x2383>
    4729:	nopl   0x0(%rax)
    4730:	xor    %r12d,%r12d
    4733:	mov    %r12d,%ecx
    4736:	shl    $0xa,%ecx
    4739:	lea    (%rcx,%rcx,2),%rbx
    473d:	lea    (%r11,%rbx,1),%rbp
    4741:	add    %r9,%rbx
    4744:	mov    $0x280,%ecx
    4749:	nopl   0x0(%rax)
    4750:	mov    %rbx,%r13
    4753:	vmovss 0x0(%rbp,%rcx,4),%xmm18
    475b:	vmovss %xmm18,(%r15,%r12,4)
    4762:	inc    %r12
    4765:	add    $0x300,%rcx
    476c:	add    $0xc00,%rbx
    4773:	cmp    $0xb80,%rcx
    477a:	jne    4750 <main_graph_model+0x23a0>
    477c:	test   %r8b,%r8b
    477f:	jne    4530 <main_graph_model+0x2180>
    4785:	data16 cs nopw 0x0(%rax,%rax,1)
    4790:	vmovss -0x3000(%r13),%xmm18
    479a:	vmovss %xmm18,(%r15,%r12,4)
    47a1:	vmovss -0x2400(%r13),%xmm18
    47ab:	vmovss %xmm18,0x4(%r15,%r12,4)
    47b3:	vmovss -0x1800(%r13),%xmm18
    47bd:	vmovss %xmm18,0x8(%r15,%r12,4)
    47c5:	vmovss -0xc00(%r13),%xmm18
    47cf:	vmovss %xmm18,0xc(%r15,%r12,4)
    47d7:	vmovss 0x0(%r13),%xmm18
    47de:	vmovss %xmm18,0x10(%r15,%r12,4)
    47e6:	vmovss 0xc00(%r13),%xmm18
    47f0:	vmovss %xmm18,0x14(%r15,%r12,4)
    47f8:	vmovss 0x1800(%r13),%xmm18
    4802:	vmovss %xmm18,0x18(%r15,%r12,4)
    480a:	vmovss 0x2400(%r13),%xmm18
    4814:	vmovss %xmm18,0x1c(%r15,%r12,4)
    481c:	add    $0x8,%r12
    4820:	add    $0x6000,%r13
    4827:	cmp    $0x93,%r12
    482e:	jne    4790 <main_graph_model+0x23e0>
    4834:	jmp    4530 <main_graph_model+0x2180>
    4839:	lea    0xb00(%r14),%rsi
    4840:	mov    %rax,%rcx
    4843:	add    $0x6e400,%rcx
    484a:	lea    0x6e400(%r14),%rdi
    4851:	cmp    %rdi,%rdx
    4854:	setae  %r8b
    4858:	cmp    %rcx,%rsi
    485b:	setae  %dil
    485f:	or     %r8b,%dil
    4862:	lea    0x4700(%r14),%r8
    4869:	xor    %r9d,%r9d
    486c:	mov    %rdx,%r10
    486f:	jmp    489c <main_graph_model+0x24ec>
    4871:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    4880:	inc    %r9
    4883:	add    $0x24c,%r10
    488a:	add    $0x4,%r14
    488e:	add    $0x4,%r8
    4892:	cmp    $0x40,%r9
    4896:	je     4b88 <main_graph_model+0x27d8>
    489c:	test   %dil,%dil
    489f:	je     4a80 <main_graph_model+0x26d0>
    48a5:	lea    (%rsi,%r9,4),%r11
    48a9:	kxnorb %k0,%k0,%k1
    48ad:	vxorps %xmm18,%xmm18,%xmm18
    48b3:	vgatherdps (%r11,%ymm0,1),%ymm18{%k1}
    48ba:	kxnorb %k0,%k0,%k1
    48be:	vxorps %xmm19,%xmm19,%xmm19
    48c4:	vgatherdps (%r11,%ymm1,1),%ymm19{%k1}
    48cb:	kxnorb %k0,%k0,%k1
    48cf:	vxorps %xmm20,%xmm20,%xmm20
    48d5:	vgatherdps (%r11,%ymm2,1),%ymm20{%k1}
    48dc:	kxnorb %k0,%k0,%k1
    48e0:	vxorps %xmm21,%xmm21,%xmm21
    48e6:	vgatherdps (%r11,%ymm3,1),%ymm21{%k1}
    48ed:	imul   $0x24c,%r9,%r15
    48f4:	vmovups %ymm18,(%rdx,%r15,1)
    48fb:	vmovups %ymm19,0x20(%rdx,%r15,1)
    4903:	vmovups %ymm20,0x40(%rdx,%r15,1)
    490b:	vmovups %ymm21,0x60(%rdx,%r15,1)
    4913:	kxnorb %k0,%k0,%k1
    4917:	vxorps %xmm18,%xmm18,%xmm18
    491d:	vgatherdps (%r11,%ymm4,1),%ymm18{%k1}
    4924:	kxnorb %k0,%k0,%k1
    4928:	vxorps %xmm19,%xmm19,%xmm19
    492e:	vgatherdps (%r11,%ymm5,1),%ymm19{%k1}
    4935:	kxnorb %k0,%k0,%k1
    4939:	vxorps %xmm20,%xmm20,%xmm20
    493f:	vgatherdps (%r11,%ymm6,1),%ymm20{%k1}
    4946:	kxnorb %k0,%k0,%k1
    494a:	vxorps %xmm21,%xmm21,%xmm21
    4950:	vgatherdps (%r11,%ymm7,1),%ymm21{%k1}
    4957:	vmovups %ymm18,0x80(%rdx,%r15,1)
    495f:	vmovups %ymm19,0xa0(%rdx,%r15,1)
    4967:	vmovups %ymm20,0xc0(%rdx,%r15,1)
    496f:	vmovups %ymm21,0xe0(%rdx,%r15,1)
    4977:	kxnorb %k0,%k0,%k1
    497b:	vxorps %xmm18,%xmm18,%xmm18
    4981:	vgatherdps (%r11,%ymm8,1),%ymm18{%k1}
    4988:	kxnorb %k0,%k0,%k1
    498c:	vxorps %xmm19,%xmm19,%xmm19
    4992:	vgatherdps (%r11,%ymm9,1),%ymm19{%k1}
    4999:	kxnorb %k0,%k0,%k1
    499d:	vxorps %xmm20,%xmm20,%xmm20
    49a3:	vgatherdps (%r11,%ymm10,1),%ymm20{%k1}
    49aa:	kxnorb %k0,%k0,%k1
    49ae:	vxorps %xmm21,%xmm21,%xmm21
    49b4:	vgatherdps (%r11,%ymm11,1),%ymm21{%k1}
    49bb:	vmovups %ymm18,0x100(%rdx,%r15,1)
    49c3:	vmovups %ymm19,0x120(%rdx,%r15,1)
    49cb:	vmovups %ymm20,0x140(%rdx,%r15,1)
    49d3:	vmovups %ymm21,0x160(%rdx,%r15,1)
    49db:	kxnorb %k0,%k0,%k1
    49df:	vxorps %xmm18,%xmm18,%xmm18
    49e5:	vgatherdps (%r11,%ymm12,1),%ymm18{%k1}
    49ec:	kxnorb %k0,%k0,%k1
    49f0:	vxorps %xmm19,%xmm19,%xmm19
    49f6:	vgatherdps (%r11,%ymm13,1),%ymm19{%k1}
    49fd:	kxnorb %k0,%k0,%k1
    4a01:	vxorps %xmm20,%xmm20,%xmm20
    4a07:	vgatherdps (%r11,%ymm14,1),%ymm20{%k1}
    4a0e:	kxnorb %k0,%k0,%k1
    4a12:	vxorps %xmm21,%xmm21,%xmm21
    4a18:	vgatherdps (%r11,%ymm15,1),%ymm21{%k1}
    4a1f:	vmovups %ymm18,0x180(%rdx,%r15,1)
    4a27:	vmovups %ymm19,0x1a0(%rdx,%r15,1)
    4a2f:	vmovups %ymm20,0x1c0(%rdx,%r15,1)
    4a37:	vmovups %ymm21,0x1e0(%rdx,%r15,1)
    4a3f:	kxnorb %k0,%k0,%k1
    4a43:	vxorps %xmm18,%xmm18,%xmm18
    4a49:	vgatherdps (%r11,%ymm16,1),%ymm18{%k1}
    4a50:	vmovups %ymm18,0x200(%rdx,%r15,1)
    4a58:	kxnorb %k0,%k0,%k1
    4a5c:	vxorps %xmm18,%xmm18,%xmm18
    4a62:	vgatherdps (%r11,%ymm17,1),%ymm18{%k1}
    4a69:	vmovups %ymm18,0x220(%rdx,%r15,1)
    4a71:	mov    $0x90,%r11d
    4a77:	jmp    4a83 <main_graph_model+0x26d3>
    4a79:	nopl   0x0(%rax)
    4a80:	xor    %r11d,%r11d
    4a83:	mov    %r11d,%ecx
    4a86:	shl    $0xa,%ecx
    4a89:	lea    (%rcx,%rcx,2),%rbx
    4a8d:	lea    (%r14,%rbx,1),%r12
    4a91:	add    %r8,%rbx
    4a94:	mov    $0x2c0,%ecx
    4a99:	nopl   0x0(%rax)
    4aa0:	mov    %rbx,%r15
    4aa3:	vmovss (%r12,%rcx,4),%xmm18
    4aaa:	vmovss %xmm18,(%r10,%r11,4)
    4ab1:	inc    %r11
    4ab4:	add    $0x300,%rcx
    4abb:	add    $0xc00,%rbx
    4ac2:	cmp    $0xbc0,%rcx
    4ac9:	jne    4aa0 <main_graph_model+0x26f0>
    4acb:	test   %dil,%dil
    4ace:	jne    4880 <main_graph_model+0x24d0>
    4ad4:	data16 data16 cs nopw 0x0(%rax,%rax,1)
    4ae0:	vmovss -0x3000(%r15),%xmm18
    4aea:	vmovss %xmm18,(%r10,%r11,4)
    4af1:	vmovss -0x2400(%r15),%xmm18
    4afb:	vmovss %xmm18,0x4(%r10,%r11,4)
    4b03:	vmovss -0x1800(%r15),%xmm18
    4b0d:	vmovss %xmm18,0x8(%r10,%r11,4)
    4b15:	vmovss -0xc00(%r15),%xmm18
    4b1f:	vmovss %xmm18,0xc(%r10,%r11,4)
    4b27:	vmovss (%r15),%xmm18
    4b2d:	vmovss %xmm18,0x10(%r10,%r11,4)
    4b35:	vmovss 0xc00(%r15),%xmm18
    4b3f:	vmovss %xmm18,0x14(%r10,%r11,4)
    4b47:	vmovss 0x1800(%r15),%xmm18
    4b51:	vmovss %xmm18,0x18(%r10,%r11,4)
    4b59:	vmovss 0x2400(%r15),%xmm18
    4b63:	vmovss %xmm18,0x1c(%r10,%r11,4)
    4b6b:	add    $0x8,%r11
    4b6f:	add    $0x6000,%r15
    4b76:	cmp    $0x93,%r11
    4b7d:	jne    4ae0 <main_graph_model+0x2730>
    4b83:	jmp    4880 <main_graph_model+0x24d0>
    4b88:	vmovaps 0x36b0(%rip),%ymm0        # 8240 <_fini+0xa5c>
    4b90:	mov    0x10(%rsp),%rcx
    4b95:	vmovups %ymm0,0x38(%rcx)
    4b9a:	vmovaps 0x36be(%rip),%ymm0        # 8260 <_fini+0xa7c>
    4ba2:	vmovups %ymm0,0x18(%rcx)
    4ba7:	mov    %rax,0x8(%rcx)
    4bab:	mov    0x8(%rsp),%rax
    4bb0:	mov    %rax,(%rcx)
    4bb3:	movq   $0x0,0x10(%rcx)
    4bbb:	mov    %rcx,%rax
    4bbe:	add    $0x18,%rsp
    4bc2:	pop    %rbx
    4bc3:	pop    %r12
    4bc5:	pop    %r13
    4bc7:	pop    %r14
    4bc9:	pop    %r15
    4bcb:	pop    %rbp
    4bcc:	vzeroupper
    4bcf:	ret
## main_graph_model@plt
    2120:	jmp    *0x8f52(%rip)        # b078 <main_graph_model@@Base+0x8cc8>
    2126:	push   $0xf
    212b:	jmp    2020 <_init+0x20>
## run_main_graph
    4ea0:	jmp    2070 <run_main_graph_model@plt>
    4ea5:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    4c30:	push   %rbp
    4c31:	mov    %rsp,%rbp
    4c34:	push   %r15
    4c36:	push   %r14
    4c38:	push   %r13
    4c3a:	push   %r12
    4c3c:	push   %rbx
    4c3d:	sub    $0x48,%rsp
    4c41:	mov    %rdi,%rbx
    4c44:	call   22b0 <omTensorListGetSize@plt>
    4c49:	cmp    $0x1,%rax
    4c4d:	jne    4e21 <run_main_graph_model+0x1f1>
    4c53:	mov    %rbx,%rdi
    4c56:	call   2150 <omTensorListGetOmtArray@plt>
    4c5b:	mov    (%rax),%r14
    4c5e:	mov    %r14,%rdi
    4c61:	call   22c0 <omTensorGetDataType@plt>
    4c66:	cmp    $0x1,%rax
    4c6a:	jne    4e36 <run_main_graph_model+0x206>
    4c70:	mov    %r14,%rdi
    4c73:	call   20f0 <omTensorGetRank@plt>
    4c78:	cmp    $0x4,%rax
    4c7c:	jne    4e63 <run_main_graph_model+0x233>
    4c82:	mov    %r14,%rdi
    4c85:	call   20e0 <omTensorGetShape@plt>
    4c8a:	mov    (%rax),%rsi
    4c8d:	cmp    $0x1,%rsi
    4c91:	jne    4e6c <run_main_graph_model+0x23c>
    4c97:	mov    0x8(%rax),%rsi
    4c9b:	cmp    $0x93,%rsi
    4ca2:	jne    4e75 <run_main_graph_model+0x245>
    4ca8:	mov    0x10(%rax),%rsi
    4cac:	cmp    $0xc,%rsi
    4cb0:	jne    4e7e <run_main_graph_model+0x24e>
    4cb6:	mov    0x18(%rax),%rsi
    4cba:	cmp    $0x40,%rsi
    4cbe:	jne    4e87 <run_main_graph_model+0x257>
    4cc4:	mov    %rbx,%rdi
    4cc7:	call   2150 <omTensorListGetOmtArray@plt>
    4ccc:	mov    %rsp,%rbx
    4ccf:	lea    -0x60(%rbx),%rcx
    4cd3:	mov    %rcx,%rsp
    4cd6:	mov    (%rax),%r14
    4cd9:	mov    %rsp,%r15
    4cdc:	lea    -0x60(%r15),%rax
    4ce0:	mov    %rax,-0x30(%rbp)
    4ce4:	mov    %rax,%rsp
    4ce7:	mov    %r14,%rdi
    4cea:	call   20a0 <omTensorGetDataPtr@plt>
    4cef:	mov    %rax,%r12
    4cf2:	mov    %r14,%rdi
    4cf5:	call   20e0 <omTensorGetShape@plt>
    4cfa:	mov    %rax,%r13
    4cfd:	mov    %r14,%rdi
    4d00:	call   2130 <omTensorGetStrides@plt>
    4d05:	mov    %r12,-0x60(%r15)
    4d09:	mov    %r12,-0x58(%r15)
    4d0d:	movq   $0x0,-0x50(%r15)
    4d15:	vmovups 0x0(%r13),%ymm0
    4d1b:	vmovups %ymm0,-0x48(%r15)
    4d21:	vmovups (%rax),%ymm0
    4d25:	vmovups %ymm0,-0x28(%r15)
    4d2b:	lea    -0x60(%rbx),%rdi
    4d2f:	mov    -0x30(%rbp),%rsi
    4d33:	vzeroupper
    4d36:	call   2160 <_mlir_ciface_main_graph_model@plt>
    4d3b:	mov    -0x60(%rbx),%r15
    4d3f:	mov    -0x58(%rbx),%r12
    4d43:	mov    -0x48(%rbx),%rax
    4d47:	mov    %rax,-0x58(%rbp)
    4d4b:	mov    -0x40(%rbx),%rax
    4d4f:	mov    %rax,-0x60(%rbp)
    4d53:	mov    -0x38(%rbx),%rax
    4d57:	mov    %rax,-0x68(%rbp)
    4d5b:	mov    -0x30(%rbx),%rax
    4d5f:	mov    %rax,-0x30(%rbp)
    4d63:	mov    -0x28(%rbx),%rax
    4d67:	mov    %rax,-0x38(%rbp)
    4d6b:	mov    -0x20(%rbx),%rax
    4d6f:	mov    %rax,-0x40(%rbp)
    4d73:	mov    -0x18(%rbx),%rax
    4d77:	mov    %rax,-0x48(%rbp)
    4d7b:	mov    -0x10(%rbx),%rax
    4d7f:	mov    %rax,-0x50(%rbp)
    4d83:	mov    %rsp,%r13
    4d86:	lea    -0x10(%r13),%rbx
    4d8a:	mov    %rbx,%rsp
    4d8d:	mov    $0x4,%edi
    4d92:	call   2290 <omTensorCreateUntyped@plt>
    4d97:	mov    %rax,%r14
    4d9a:	mov    $0x1,%esi
    4d9f:	mov    %rax,%rdi
    4da2:	mov    %r15,%rdx
    4da5:	mov    %r12,%rcx
    4da8:	call   20d0 <omTensorSetDataPtr@plt>
    4dad:	mov    $0x1,%esi
    4db2:	mov    %r14,%rdi
    4db5:	call   22a0 <omTensorSetDataType@plt>
    4dba:	mov    %r14,%rdi
    4dbd:	call   20e0 <omTensorGetShape@plt>
    4dc2:	mov    %rax,%r15
    4dc5:	mov    %r14,%rdi
    4dc8:	call   2130 <omTensorGetStrides@plt>
    4dcd:	mov    -0x58(%rbp),%rcx
    4dd1:	mov    %rcx,(%r15)
    4dd4:	mov    -0x38(%rbp),%rcx
    4dd8:	mov    %rcx,(%rax)
    4ddb:	mov    -0x60(%rbp),%rcx
    4ddf:	mov    %rcx,0x8(%r15)
    4de3:	mov    -0x40(%rbp),%rcx
    4de7:	mov    %rcx,0x8(%rax)
    4deb:	mov    -0x68(%rbp),%rcx
    4def:	mov    %rcx,0x10(%r15)
    4df3:	mov    -0x48(%rbp),%rcx
    4df7:	mov    %rcx,0x10(%rax)
    4dfb:	mov    -0x30(%rbp),%rcx
    4dff:	mov    %rcx,0x18(%r15)
    4e03:	mov    -0x50(%rbp),%rcx
    4e07:	mov    %rcx,0x18(%rax)
    4e0b:	mov    %r14,-0x10(%r13)
    4e0f:	mov    $0x1,%esi
    4e14:	mov    %rbx,%rdi
    4e17:	call   21d0 <omTensorListCreate@plt>
    4e1c:	mov    %rax,%rbx
    4e1f:	jmp    4e51 <run_main_graph_model+0x221>
    4e21:	lea    0x36e8(%rip),%rdi        # 8510 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    4e28:	xor    %ebx,%ebx
    4e2a:	mov    %rax,%rsi
    4e2d:	xor    %eax,%eax
    4e2f:	call   2040 <printf@plt>
    4e34:	jmp    4e46 <run_main_graph_model+0x216>
    4e36:	lea    0x36a3(%rip),%rdi        # 84e0 <om_Wrong data type for the input 0: expect f32^J_model>
    4e3d:	xor    %ebx,%ebx
    4e3f:	xor    %eax,%eax
    4e41:	call   2040 <printf@plt>
    4e46:	call   2030 <__errno_location@plt>
    4e4b:	movl   $0x16,(%rax)
    4e51:	mov    %rbx,%rax
    4e54:	lea    -0x28(%rbp),%rsp
    4e58:	pop    %rbx
    4e59:	pop    %r12
    4e5b:	pop    %r13
    4e5d:	pop    %r14
    4e5f:	pop    %r15
    4e61:	pop    %rbp
    4e62:	ret
    4e63:	lea    0x3636(%rip),%rdi        # 84a0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    4e6a:	jmp    4e28 <run_main_graph_model+0x1f8>
    4e6c:	lea    0x35dd(%rip),%rdi        # 8450 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    4e73:	jmp    4e8e <run_main_graph_model+0x25e>
    4e75:	lea    0x3584(%rip),%rdi        # 8400 <om_Wrong size for the dimension 1 of the input 0: expect 147, but got %lld^J_model>
    4e7c:	jmp    4e8e <run_main_graph_model+0x25e>
    4e7e:	lea    0x352b(%rip),%rdi        # 83b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    4e85:	jmp    4e8e <run_main_graph_model+0x25e>
    4e87:	lea    0x34d2(%rip),%rdi        # 8360 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    4e8e:	xor    %ebx,%ebx
    4e90:	jmp    4e2d <run_main_graph_model+0x1fd>
    4e92:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model@plt
    2070:	jmp    *0x8faa(%rip)        # b020 <run_main_graph_model@@Base+0x63f0>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
