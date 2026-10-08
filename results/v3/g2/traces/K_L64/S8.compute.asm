## _mlir_ciface_main_graph_model
    2f90:	push   %rbx
    2f91:	sub    $0x90,%rsp
    2f98:	mov    %rdi,%rbx
    2f9b:	mov    0x8(%rsi),%rdx
    2f9f:	lea    0x38(%rsp),%rdi
    2fa4:	call   2120 <main_graph_model@plt>
    2fa9:	mov    0x88(%rsp),%rax
    2fb1:	vmovups 0x78(%rsp),%xmm0
    2fb7:	vmovups 0x38(%rsp),%ymm1
    2fbd:	vmovups 0x58(%rsp),%ymm2
    2fc3:	vmovups %ymm1,(%rbx)
    2fc7:	vmovups %ymm2,0x20(%rbx)
    2fcc:	vmovups %xmm0,0x40(%rbx)
    2fd1:	mov    %rax,0x50(%rbx)
    2fd5:	add    $0x90,%rsp
    2fdc:	pop    %rbx
    2fdd:	vzeroupper
    2fe0:	ret
    2fe1:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2160:	jmp    *0x6f32(%rip)        # 9098 <_mlir_ciface_main_graph_model@@Base+0x6108>
    2166:	push   $0x13
    216b:	jmp    2020 <_init+0x20>
## main_graph_model
    23b0:	push   %r14
    23b2:	push   %rbx
    23b3:	push   %rax
    23b4:	mov    %rdx,%r14
    23b7:	mov    %rdi,%rbx
    23ba:	mov    $0x30010,%edi
    23bf:	call   21e0 <malloc@plt>
    23c4:	mov    %rax,%rcx
    23c7:	add    $0xf,%rax
    23cb:	and    $0xfffffffffffffff0,%rax
    23cf:	lea    0x30000(%rax),%rdx
    23d6:	vpbroadcastq %r14,%zmm0
    23dc:	vpaddq 0x3c1a(%rip),%zmm0,%zmm1        # 6000 <_fini+0x46c>
    23e6:	vpaddq 0x3c50(%rip),%zmm0,%zmm2        # 6040 <_fini+0x4ac>
    23f0:	vpaddq 0x3c86(%rip),%zmm0,%zmm3        # 6080 <_fini+0x4ec>
    23fa:	vpaddq 0x3cbc(%rip),%zmm0,%zmm4        # 60c0 <_fini+0x52c>
    2404:	vpaddq 0x3cf2(%rip),%zmm0,%zmm5        # 6100 <_fini+0x56c>
    240e:	vpaddq 0x3d28(%rip),%zmm0,%zmm0        # 6140 <_fini+0x5ac>
    2418:	lea    0x1800(%r14),%rsi
    241f:	lea    0xc00(%r14),%rdi
    2426:	vpbroadcastq %rax,%zmm6
    242c:	vpbroadcastq %rdx,%zmm7
    2432:	vpcmpnleuq %zmm6,%zmm2,%k0
    2439:	vpcmpnleuq %zmm6,%zmm1,%k1
    2440:	kunpckbw %k0,%k1,%k0
    2444:	vpcmpltuq %zmm7,%zmm4,%k1
    244b:	vpcmpltuq %zmm7,%zmm3,%k2
    2452:	kunpckbw %k1,%k2,%k1
    2456:	kandw  %k1,%k0,%k0
    245a:	vpcmpnleuq %zmm6,%zmm5,%k1
    2461:	cmp    %rax,%rsi
    2464:	seta   %sil
    2468:	cmp    %rdx,%rdi
    246b:	setb   %r8b
    246f:	vpcmpltuq %zmm7,%zmm0,%k1{%k1}
    2476:	and    %sil,%r8b
    2479:	cmp    %rax,%rdi
    247c:	seta   %sil
    2480:	cmp    %rdx,%r14
    2483:	setb   %dl
    2486:	and    %sil,%dl
    2489:	korw   %k1,%k0,%k1
    248d:	vpmovm2w %k0,%ymm0
    2493:	vpmovm2w %k1,%ymm1
    2499:	vpblendd $0xf0,%ymm0,%ymm1,%ymm0
    249f:	vpmovw2m %ymm0,%k0
    24a5:	kortestw %k0,%k0
    24a9:	setne  %sil
    24ad:	or     %r8b,%dl
    24b0:	or     %sil,%dl
    24b3:	add    $0x2f400,%r14
    24ba:	lea    0xfc(%rax),%rsi
    24c1:	xor    %edi,%edi
    24c3:	vmovdqa64 0x3cb3(%rip),%zmm0        # 6180 <_fini+0x5ec>
    24cd:	vpbroadcastq 0x4121(%rip),%zmm1        # 65f8 <_entry_point_0_out_sig_model+0x48>
    24d7:	jmp    24fb <main_graph_model+0x14b>
    24d9:	nopl   0x0(%rax)
    24e0:	inc    %rdi
    24e3:	add    $0x100,%r14
    24ea:	add    $0x4000,%rsi
    24f1:	cmp    $0xc,%rdi
    24f5:	je     2f4b <main_graph_model+0xb9b>
    24fb:	test   %dl,%dl
    24fd:	je     2990 <main_graph_model+0x5e0>
    2503:	mov    %rsi,%r8
    2506:	xor    %r9d,%r9d
    2509:	nopl   0x0(%rax)
    2510:	vmovss -0x2f400(%r14,%r9,4),%xmm2
    251a:	vmovss %xmm2,-0xfc(%r8)
    2523:	vmovss -0x2e800(%r14,%r9,4),%xmm2
    252d:	vmovss %xmm2,-0xf8(%r8)
    2536:	vmovss -0x2dc00(%r14,%r9,4),%xmm2
    2540:	vmovss %xmm2,-0xf4(%r8)
    2549:	vmovss -0x2d000(%r14,%r9,4),%xmm2
    2553:	vmovss %xmm2,-0xf0(%r8)
    255c:	vmovss -0x2c400(%r14,%r9,4),%xmm2
    2566:	vmovss %xmm2,-0xec(%r8)
    256f:	vmovss -0x2b800(%r14,%r9,4),%xmm2
    2579:	vmovss %xmm2,-0xe8(%r8)
    2582:	vmovss -0x2ac00(%r14,%r9,4),%xmm2
    258c:	vmovss %xmm2,-0xe4(%r8)
    2595:	vmovss -0x2a000(%r14,%r9,4),%xmm2
    259f:	vmovss %xmm2,-0xe0(%r8)
    25a8:	vmovss -0x29400(%r14,%r9,4),%xmm2
    25b2:	vmovss %xmm2,-0xdc(%r8)
    25bb:	vmovss -0x28800(%r14,%r9,4),%xmm2
    25c5:	vmovss %xmm2,-0xd8(%r8)
    25ce:	vmovss -0x27c00(%r14,%r9,4),%xmm2
    25d8:	vmovss %xmm2,-0xd4(%r8)
    25e1:	vmovss -0x27000(%r14,%r9,4),%xmm2
    25eb:	vmovss %xmm2,-0xd0(%r8)
    25f4:	vmovss -0x26400(%r14,%r9,4),%xmm2
    25fe:	vmovss %xmm2,-0xcc(%r8)
    2607:	vmovss -0x25800(%r14,%r9,4),%xmm2
    2611:	vmovss %xmm2,-0xc8(%r8)
    261a:	vmovss -0x24c00(%r14,%r9,4),%xmm2
    2624:	vmovss %xmm2,-0xc4(%r8)
    262d:	vmovss -0x24000(%r14,%r9,4),%xmm2
    2637:	vmovss %xmm2,-0xc0(%r8)
    2640:	vmovss -0x23400(%r14,%r9,4),%xmm2
    264a:	vmovss %xmm2,-0xbc(%r8)
    2653:	vmovss -0x22800(%r14,%r9,4),%xmm2
    265d:	vmovss %xmm2,-0xb8(%r8)
    2666:	vmovss -0x21c00(%r14,%r9,4),%xmm2
    2670:	vmovss %xmm2,-0xb4(%r8)
    2679:	vmovss -0x21000(%r14,%r9,4),%xmm2
    2683:	vmovss %xmm2,-0xb0(%r8)
    268c:	vmovss -0x20400(%r14,%r9,4),%xmm2
    2696:	vmovss %xmm2,-0xac(%r8)
    269f:	vmovss -0x1f800(%r14,%r9,4),%xmm2
    26a9:	vmovss %xmm2,-0xa8(%r8)
    26b2:	vmovss -0x1ec00(%r14,%r9,4),%xmm2
    26bc:	vmovss %xmm2,-0xa4(%r8)
    26c5:	vmovss -0x1e000(%r14,%r9,4),%xmm2
    26cf:	vmovss %xmm2,-0xa0(%r8)
    26d8:	vmovss -0x1d400(%r14,%r9,4),%xmm2
    26e2:	vmovss %xmm2,-0x9c(%r8)
    26eb:	vmovss -0x1c800(%r14,%r9,4),%xmm2
    26f5:	vmovss %xmm2,-0x98(%r8)
    26fe:	vmovss -0x1bc00(%r14,%r9,4),%xmm2
    2708:	vmovss %xmm2,-0x94(%r8)
    2711:	vmovss -0x1b000(%r14,%r9,4),%xmm2
    271b:	vmovss %xmm2,-0x90(%r8)
    2724:	vmovss -0x1a400(%r14,%r9,4),%xmm2
    272e:	vmovss %xmm2,-0x8c(%r8)
    2737:	vmovss -0x19800(%r14,%r9,4),%xmm2
    2741:	vmovss %xmm2,-0x88(%r8)
    274a:	vmovss -0x18c00(%r14,%r9,4),%xmm2
    2754:	vmovss %xmm2,-0x84(%r8)
    275d:	vmovss -0x18000(%r14,%r9,4),%xmm2
    2767:	vmovss %xmm2,-0x80(%r8)
    276d:	vmovss -0x17400(%r14,%r9,4),%xmm2
    2777:	vmovss %xmm2,-0x7c(%r8)
    277d:	vmovss -0x16800(%r14,%r9,4),%xmm2
    2787:	vmovss %xmm2,-0x78(%r8)
    278d:	vmovss -0x15c00(%r14,%r9,4),%xmm2
    2797:	vmovss %xmm2,-0x74(%r8)
    279d:	vmovss -0x15000(%r14,%r9,4),%xmm2
    27a7:	vmovss %xmm2,-0x70(%r8)
    27ad:	vmovss -0x14400(%r14,%r9,4),%xmm2
    27b7:	vmovss %xmm2,-0x6c(%r8)
    27bd:	vmovss -0x13800(%r14,%r9,4),%xmm2
    27c7:	vmovss %xmm2,-0x68(%r8)
    27cd:	vmovss -0x12c00(%r14,%r9,4),%xmm2
    27d7:	vmovss %xmm2,-0x64(%r8)
    27dd:	vmovss -0x12000(%r14,%r9,4),%xmm2
    27e7:	vmovss %xmm2,-0x60(%r8)
    27ed:	vmovss -0x11400(%r14,%r9,4),%xmm2
    27f7:	vmovss %xmm2,-0x5c(%r8)
    27fd:	vmovss -0x10800(%r14,%r9,4),%xmm2
    2807:	vmovss %xmm2,-0x58(%r8)
    280d:	vmovss -0xfc00(%r14,%r9,4),%xmm2
    2817:	vmovss %xmm2,-0x54(%r8)
    281d:	vmovss -0xf000(%r14,%r9,4),%xmm2
    2827:	vmovss %xmm2,-0x50(%r8)
    282d:	vmovss -0xe400(%r14,%r9,4),%xmm2
    2837:	vmovss %xmm2,-0x4c(%r8)
    283d:	vmovss -0xd800(%r14,%r9,4),%xmm2
    2847:	vmovss %xmm2,-0x48(%r8)
    284d:	vmovss -0xcc00(%r14,%r9,4),%xmm2
    2857:	vmovss %xmm2,-0x44(%r8)
    285d:	vmovss -0xc000(%r14,%r9,4),%xmm2
    2867:	vmovss %xmm2,-0x40(%r8)
    286d:	vmovss -0xb400(%r14,%r9,4),%xmm2
    2877:	vmovss %xmm2,-0x3c(%r8)
    287d:	vmovss -0xa800(%r14,%r9,4),%xmm2
    2887:	vmovss %xmm2,-0x38(%r8)
    288d:	vmovss -0x9c00(%r14,%r9,4),%xmm2
    2897:	vmovss %xmm2,-0x34(%r8)
    289d:	vmovss -0x9000(%r14,%r9,4),%xmm2
    28a7:	vmovss %xmm2,-0x30(%r8)
    28ad:	vmovss -0x8400(%r14,%r9,4),%xmm2
    28b7:	vmovss %xmm2,-0x2c(%r8)
    28bd:	vmovss -0x7800(%r14,%r9,4),%xmm2
    28c7:	vmovss %xmm2,-0x28(%r8)
    28cd:	vmovss -0x6c00(%r14,%r9,4),%xmm2
    28d7:	vmovss %xmm2,-0x24(%r8)
    28dd:	vmovss -0x6000(%r14,%r9,4),%xmm2
    28e7:	vmovss %xmm2,-0x20(%r8)
    28ed:	vmovss -0x5400(%r14,%r9,4),%xmm2
    28f7:	vmovss %xmm2,-0x1c(%r8)
    28fd:	vmovss -0x4800(%r14,%r9,4),%xmm2
    2907:	vmovss %xmm2,-0x18(%r8)
    290d:	vmovss -0x3c00(%r14,%r9,4),%xmm2
    2917:	vmovss %xmm2,-0x14(%r8)
    291d:	vmovss -0x3000(%r14,%r9,4),%xmm2
    2927:	vmovss %xmm2,-0x10(%r8)
    292d:	vmovss -0x2400(%r14,%r9,4),%xmm2
    2937:	vmovss %xmm2,-0xc(%r8)
    293d:	vmovss -0x1800(%r14,%r9,4),%xmm2
    2947:	vmovss %xmm2,-0x8(%r8)
    294d:	vmovss -0xc00(%r14,%r9,4),%xmm2
    2957:	vmovss %xmm2,-0x4(%r8)
    295d:	vmovd  (%r14,%r9,4),%xmm2
    2963:	vmovd  %xmm2,(%r8)
    2968:	inc    %r9
    296b:	add    $0x100,%r8
    2972:	cmp    $0x40,%r9
    2976:	jne    2510 <main_graph_model+0x160>
    297c:	jmp    24e0 <main_graph_model+0x130>
    2981:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2990:	mov    %rdi,%r8
    2993:	shl    $0xe,%r8
    2997:	add    %rax,%r8
    299a:	xor    %r9d,%r9d
    299d:	vmovdqa64 %zmm0,%zmm2
    29a3:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    29b0:	vpsllq $0x8,%zmm2,%zmm3
    29b7:	vmovups -0x2f400(%r14,%r9,4),%ymm4
    29c1:	kxnorb %k0,%k0,%k1
    29c5:	vscatterqps %ymm4,(%r8,%zmm3,1){%k1}
    29cc:	vmovups -0x2e800(%r14,%r9,4),%ymm4
    29d6:	kxnorb %k0,%k0,%k1
    29da:	vscatterqps %ymm4,0x4(%r8,%zmm3,1){%k1}
    29e2:	vmovups -0x2dc00(%r14,%r9,4),%ymm4
    29ec:	kxnorb %k0,%k0,%k1
    29f0:	vscatterqps %ymm4,0x8(%r8,%zmm3,1){%k1}
    29f8:	vmovups -0x2d000(%r14,%r9,4),%ymm4
    2a02:	kxnorb %k0,%k0,%k1
    2a06:	vscatterqps %ymm4,0xc(%r8,%zmm3,1){%k1}
    2a0e:	vmovups -0x2c400(%r14,%r9,4),%ymm4
    2a18:	kxnorb %k0,%k0,%k1
    2a1c:	vscatterqps %ymm4,0x10(%r8,%zmm3,1){%k1}
    2a24:	vmovups -0x2b800(%r14,%r9,4),%ymm4
    2a2e:	kxnorb %k0,%k0,%k1
    2a32:	vscatterqps %ymm4,0x14(%r8,%zmm3,1){%k1}
    2a3a:	vmovups -0x2ac00(%r14,%r9,4),%ymm4
    2a44:	kxnorb %k0,%k0,%k1
    2a48:	vscatterqps %ymm4,0x18(%r8,%zmm3,1){%k1}
    2a50:	vmovups -0x2a000(%r14,%r9,4),%ymm4
    2a5a:	kxnorb %k0,%k0,%k1
    2a5e:	vscatterqps %ymm4,0x1c(%r8,%zmm3,1){%k1}
    2a66:	vmovups -0x29400(%r14,%r9,4),%ymm4
    2a70:	kxnorb %k0,%k0,%k1
    2a74:	vscatterqps %ymm4,0x20(%r8,%zmm3,1){%k1}
    2a7c:	vmovups -0x28800(%r14,%r9,4),%ymm4
    2a86:	kxnorb %k0,%k0,%k1
    2a8a:	vscatterqps %ymm4,0x24(%r8,%zmm3,1){%k1}
    2a92:	vmovups -0x27c00(%r14,%r9,4),%ymm4
    2a9c:	kxnorb %k0,%k0,%k1
    2aa0:	vscatterqps %ymm4,0x28(%r8,%zmm3,1){%k1}
    2aa8:	vmovups -0x27000(%r14,%r9,4),%ymm4
    2ab2:	kxnorb %k0,%k0,%k1
    2ab6:	vscatterqps %ymm4,0x2c(%r8,%zmm3,1){%k1}
    2abe:	vmovups -0x26400(%r14,%r9,4),%ymm4
    2ac8:	kxnorb %k0,%k0,%k1
    2acc:	vscatterqps %ymm4,0x30(%r8,%zmm3,1){%k1}
    2ad4:	vmovups -0x25800(%r14,%r9,4),%ymm4
    2ade:	kxnorb %k0,%k0,%k1
    2ae2:	vscatterqps %ymm4,0x34(%r8,%zmm3,1){%k1}
    2aea:	vmovups -0x24c00(%r14,%r9,4),%ymm4
    2af4:	kxnorb %k0,%k0,%k1
    2af8:	vscatterqps %ymm4,0x38(%r8,%zmm3,1){%k1}
    2b00:	vmovups -0x24000(%r14,%r9,4),%ymm4
    2b0a:	kxnorb %k0,%k0,%k1
    2b0e:	vscatterqps %ymm4,0x3c(%r8,%zmm3,1){%k1}
    2b16:	vmovups -0x23400(%r14,%r9,4),%ymm4
    2b20:	kxnorb %k0,%k0,%k1
    2b24:	vscatterqps %ymm4,0x40(%r8,%zmm3,1){%k1}
    2b2c:	vmovups -0x22800(%r14,%r9,4),%ymm4
    2b36:	kxnorb %k0,%k0,%k1
    2b3a:	vscatterqps %ymm4,0x44(%r8,%zmm3,1){%k1}
    2b42:	vmovups -0x21c00(%r14,%r9,4),%ymm4
    2b4c:	kxnorb %k0,%k0,%k1
    2b50:	vscatterqps %ymm4,0x48(%r8,%zmm3,1){%k1}
    2b58:	vmovups -0x21000(%r14,%r9,4),%ymm4
    2b62:	kxnorb %k0,%k0,%k1
    2b66:	vscatterqps %ymm4,0x4c(%r8,%zmm3,1){%k1}
    2b6e:	vmovups -0x20400(%r14,%r9,4),%ymm4
    2b78:	kxnorb %k0,%k0,%k1
    2b7c:	vscatterqps %ymm4,0x50(%r8,%zmm3,1){%k1}
    2b84:	vmovups -0x1f800(%r14,%r9,4),%ymm4
    2b8e:	kxnorb %k0,%k0,%k1
    2b92:	vscatterqps %ymm4,0x54(%r8,%zmm3,1){%k1}
    2b9a:	vmovups -0x1ec00(%r14,%r9,4),%ymm4
    2ba4:	kxnorb %k0,%k0,%k1
    2ba8:	vscatterqps %ymm4,0x58(%r8,%zmm3,1){%k1}
    2bb0:	vmovups -0x1e000(%r14,%r9,4),%ymm4
    2bba:	kxnorb %k0,%k0,%k1
    2bbe:	vscatterqps %ymm4,0x5c(%r8,%zmm3,1){%k1}
    2bc6:	vmovups -0x1d400(%r14,%r9,4),%ymm4
    2bd0:	kxnorb %k0,%k0,%k1
    2bd4:	vscatterqps %ymm4,0x60(%r8,%zmm3,1){%k1}
    2bdc:	vmovups -0x1c800(%r14,%r9,4),%ymm4
    2be6:	kxnorb %k0,%k0,%k1
    2bea:	vscatterqps %ymm4,0x64(%r8,%zmm3,1){%k1}
    2bf2:	vmovups -0x1bc00(%r14,%r9,4),%ymm4
    2bfc:	kxnorb %k0,%k0,%k1
    2c00:	vscatterqps %ymm4,0x68(%r8,%zmm3,1){%k1}
    2c08:	vmovups -0x1b000(%r14,%r9,4),%ymm4
    2c12:	kxnorb %k0,%k0,%k1
    2c16:	vscatterqps %ymm4,0x6c(%r8,%zmm3,1){%k1}
    2c1e:	vmovups -0x1a400(%r14,%r9,4),%ymm4
    2c28:	kxnorb %k0,%k0,%k1
    2c2c:	vscatterqps %ymm4,0x70(%r8,%zmm3,1){%k1}
    2c34:	vmovups -0x19800(%r14,%r9,4),%ymm4
    2c3e:	kxnorb %k0,%k0,%k1
    2c42:	vscatterqps %ymm4,0x74(%r8,%zmm3,1){%k1}
    2c4a:	vmovups -0x18c00(%r14,%r9,4),%ymm4
    2c54:	kxnorb %k0,%k0,%k1
    2c58:	vscatterqps %ymm4,0x78(%r8,%zmm3,1){%k1}
    2c60:	vmovups -0x18000(%r14,%r9,4),%ymm4
    2c6a:	kxnorb %k0,%k0,%k1
    2c6e:	vscatterqps %ymm4,0x7c(%r8,%zmm3,1){%k1}
    2c76:	vmovups -0x17400(%r14,%r9,4),%ymm4
    2c80:	kxnorb %k0,%k0,%k1
    2c84:	vscatterqps %ymm4,0x80(%r8,%zmm3,1){%k1}
    2c8c:	vmovups -0x16800(%r14,%r9,4),%ymm4
    2c96:	kxnorb %k0,%k0,%k1
    2c9a:	vscatterqps %ymm4,0x84(%r8,%zmm3,1){%k1}
    2ca2:	vmovups -0x15c00(%r14,%r9,4),%ymm4
    2cac:	kxnorb %k0,%k0,%k1
    2cb0:	vscatterqps %ymm4,0x88(%r8,%zmm3,1){%k1}
    2cb8:	vmovups -0x15000(%r14,%r9,4),%ymm4
    2cc2:	kxnorb %k0,%k0,%k1
    2cc6:	vscatterqps %ymm4,0x8c(%r8,%zmm3,1){%k1}
    2cce:	vmovups -0x14400(%r14,%r9,4),%ymm4
    2cd8:	kxnorb %k0,%k0,%k1
    2cdc:	vscatterqps %ymm4,0x90(%r8,%zmm3,1){%k1}
    2ce4:	vmovups -0x13800(%r14,%r9,4),%ymm4
    2cee:	kxnorb %k0,%k0,%k1
    2cf2:	vscatterqps %ymm4,0x94(%r8,%zmm3,1){%k1}
    2cfa:	vmovups -0x12c00(%r14,%r9,4),%ymm4
    2d04:	kxnorb %k0,%k0,%k1
    2d08:	vscatterqps %ymm4,0x98(%r8,%zmm3,1){%k1}
    2d10:	vmovups -0x12000(%r14,%r9,4),%ymm4
    2d1a:	kxnorb %k0,%k0,%k1
    2d1e:	vscatterqps %ymm4,0x9c(%r8,%zmm3,1){%k1}
    2d26:	vmovups -0x11400(%r14,%r9,4),%ymm4
    2d30:	kxnorb %k0,%k0,%k1
    2d34:	vscatterqps %ymm4,0xa0(%r8,%zmm3,1){%k1}
    2d3c:	vmovups -0x10800(%r14,%r9,4),%ymm4
    2d46:	kxnorb %k0,%k0,%k1
    2d4a:	vscatterqps %ymm4,0xa4(%r8,%zmm3,1){%k1}
    2d52:	vmovups -0xfc00(%r14,%r9,4),%ymm4
    2d5c:	kxnorb %k0,%k0,%k1
    2d60:	vscatterqps %ymm4,0xa8(%r8,%zmm3,1){%k1}
    2d68:	vmovups -0xf000(%r14,%r9,4),%ymm4
    2d72:	kxnorb %k0,%k0,%k1
    2d76:	vscatterqps %ymm4,0xac(%r8,%zmm3,1){%k1}
    2d7e:	vmovups -0xe400(%r14,%r9,4),%ymm4
    2d88:	kxnorb %k0,%k0,%k1
    2d8c:	vscatterqps %ymm4,0xb0(%r8,%zmm3,1){%k1}
    2d94:	vmovups -0xd800(%r14,%r9,4),%ymm4
    2d9e:	kxnorb %k0,%k0,%k1
    2da2:	vscatterqps %ymm4,0xb4(%r8,%zmm3,1){%k1}
    2daa:	vmovups -0xcc00(%r14,%r9,4),%ymm4
    2db4:	kxnorb %k0,%k0,%k1
    2db8:	vscatterqps %ymm4,0xb8(%r8,%zmm3,1){%k1}
    2dc0:	vmovups -0xc000(%r14,%r9,4),%ymm4
    2dca:	kxnorb %k0,%k0,%k1
    2dce:	vscatterqps %ymm4,0xbc(%r8,%zmm3,1){%k1}
    2dd6:	vmovups -0xb400(%r14,%r9,4),%ymm4
    2de0:	kxnorb %k0,%k0,%k1
    2de4:	vscatterqps %ymm4,0xc0(%r8,%zmm3,1){%k1}
    2dec:	vmovups -0xa800(%r14,%r9,4),%ymm4
    2df6:	kxnorb %k0,%k0,%k1
    2dfa:	vscatterqps %ymm4,0xc4(%r8,%zmm3,1){%k1}
    2e02:	vmovups -0x9c00(%r14,%r9,4),%ymm4
    2e0c:	kxnorb %k0,%k0,%k1
    2e10:	vscatterqps %ymm4,0xc8(%r8,%zmm3,1){%k1}
    2e18:	vmovups -0x9000(%r14,%r9,4),%ymm4
    2e22:	kxnorb %k0,%k0,%k1
    2e26:	vscatterqps %ymm4,0xcc(%r8,%zmm3,1){%k1}
    2e2e:	vmovups -0x8400(%r14,%r9,4),%ymm4
    2e38:	kxnorb %k0,%k0,%k1
    2e3c:	vscatterqps %ymm4,0xd0(%r8,%zmm3,1){%k1}
    2e44:	vmovups -0x7800(%r14,%r9,4),%ymm4
    2e4e:	kxnorb %k0,%k0,%k1
    2e52:	vscatterqps %ymm4,0xd4(%r8,%zmm3,1){%k1}
    2e5a:	vmovups -0x6c00(%r14,%r9,4),%ymm4
    2e64:	kxnorb %k0,%k0,%k1
    2e68:	vscatterqps %ymm4,0xd8(%r8,%zmm3,1){%k1}
    2e70:	vmovups -0x6000(%r14,%r9,4),%ymm4
    2e7a:	kxnorb %k0,%k0,%k1
    2e7e:	vscatterqps %ymm4,0xdc(%r8,%zmm3,1){%k1}
    2e86:	vmovups -0x5400(%r14,%r9,4),%ymm4
    2e90:	kxnorb %k0,%k0,%k1
    2e94:	vscatterqps %ymm4,0xe0(%r8,%zmm3,1){%k1}
    2e9c:	vmovups -0x4800(%r14,%r9,4),%ymm4
    2ea6:	kxnorb %k0,%k0,%k1
    2eaa:	vscatterqps %ymm4,0xe4(%r8,%zmm3,1){%k1}
    2eb2:	vmovups -0x3c00(%r14,%r9,4),%ymm4
    2ebc:	kxnorb %k0,%k0,%k1
    2ec0:	vscatterqps %ymm4,0xe8(%r8,%zmm3,1){%k1}
    2ec8:	vmovups -0x3000(%r14,%r9,4),%ymm4
    2ed2:	kxnorb %k0,%k0,%k1
    2ed6:	vscatterqps %ymm4,0xec(%r8,%zmm3,1){%k1}
    2ede:	vmovups -0x2400(%r14,%r9,4),%ymm4
    2ee8:	kxnorb %k0,%k0,%k1
    2eec:	vscatterqps %ymm4,0xf0(%r8,%zmm3,1){%k1}
    2ef4:	vmovups -0x1800(%r14,%r9,4),%ymm4
    2efe:	kxnorb %k0,%k0,%k1
    2f02:	vscatterqps %ymm4,0xf4(%r8,%zmm3,1){%k1}
    2f0a:	vmovups -0xc00(%r14,%r9,4),%ymm4
    2f14:	kxnorb %k0,%k0,%k1
    2f18:	vscatterqps %ymm4,0xf8(%r8,%zmm3,1){%k1}
    2f20:	vmovups (%r14,%r9,4),%ymm4
    2f26:	kxnorb %k0,%k0,%k1
    2f2a:	vscatterqps %ymm4,0xfc(%r8,%zmm3,1){%k1}
    2f32:	add    $0x8,%r9
    2f36:	vpaddq %zmm1,%zmm2,%zmm2
    2f3c:	cmp    $0x40,%r9
    2f40:	jne    29b0 <main_graph_model+0x600>
    2f46:	jmp    24e0 <main_graph_model+0x130>
    2f4b:	vmovaps 0x36ad(%rip),%ymm0        # 6600 <_entry_point_0_out_sig_model+0x50>
    2f53:	vmovups %ymm0,0x38(%rbx)
    2f58:	vmovaps 0x36c0(%rip),%ymm0        # 6620 <_entry_point_0_out_sig_model+0x70>
    2f60:	vmovups %ymm0,0x18(%rbx)
    2f65:	mov    %rax,0x8(%rbx)
    2f69:	mov    %rcx,(%rbx)
    2f6c:	movq   $0x0,0x10(%rbx)
    2f74:	mov    %rbx,%rax
    2f77:	add    $0x8,%rsp
    2f7b:	pop    %rbx
    2f7c:	pop    %r14
    2f7e:	vzeroupper
    2f81:	ret
    2f82:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## main_graph_model@plt
    2120:	jmp    *0x6f52(%rip)        # 9078 <main_graph_model@@Base+0x6cc8>
    2126:	push   $0xf
    212b:	jmp    2020 <_init+0x20>
## run_main_graph
    3250:	jmp    2070 <run_main_graph_model@plt>
    3255:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    2ff0:	push   %rbp
    2ff1:	mov    %rsp,%rbp
    2ff4:	push   %r15
    2ff6:	push   %r14
    2ff8:	push   %r13
    2ffa:	push   %r12
    2ffc:	push   %rbx
    2ffd:	sub    $0x48,%rsp
    3001:	mov    %rdi,%rbx
    3004:	call   22b0 <omTensorListGetSize@plt>
    3009:	cmp    $0x1,%rax
    300d:	jne    31de <run_main_graph_model+0x1ee>
    3013:	mov    %rbx,%rdi
    3016:	call   2150 <omTensorListGetOmtArray@plt>
    301b:	mov    (%rax),%r14
    301e:	mov    %r14,%rdi
    3021:	call   22c0 <omTensorGetDataType@plt>
    3026:	cmp    $0x1,%rax
    302a:	jne    31f3 <run_main_graph_model+0x203>
    3030:	mov    %r14,%rdi
    3033:	call   20f0 <omTensorGetRank@plt>
    3038:	cmp    $0x4,%rax
    303c:	jne    3220 <run_main_graph_model+0x230>
    3042:	mov    %r14,%rdi
    3045:	call   20e0 <omTensorGetShape@plt>
    304a:	mov    (%rax),%rsi
    304d:	cmp    $0x1,%rsi
    3051:	jne    3229 <run_main_graph_model+0x239>
    3057:	mov    0x8(%rax),%rsi
    305b:	cmp    $0x40,%rsi
    305f:	jne    3232 <run_main_graph_model+0x242>
    3065:	mov    0x10(%rax),%rsi
    3069:	cmp    $0xc,%rsi
    306d:	jne    323b <run_main_graph_model+0x24b>
    3073:	mov    0x18(%rax),%rsi
    3077:	cmp    $0x40,%rsi
    307b:	jne    3244 <run_main_graph_model+0x254>
    3081:	mov    %rbx,%rdi
    3084:	call   2150 <omTensorListGetOmtArray@plt>
    3089:	mov    %rsp,%rbx
    308c:	lea    -0x60(%rbx),%rcx
    3090:	mov    %rcx,%rsp
    3093:	mov    (%rax),%r14
    3096:	mov    %rsp,%r15
    3099:	lea    -0x60(%r15),%rax
    309d:	mov    %rax,-0x30(%rbp)
    30a1:	mov    %rax,%rsp
    30a4:	mov    %r14,%rdi
    30a7:	call   20a0 <omTensorGetDataPtr@plt>
    30ac:	mov    %rax,%r12
    30af:	mov    %r14,%rdi
    30b2:	call   20e0 <omTensorGetShape@plt>
    30b7:	mov    %rax,%r13
    30ba:	mov    %r14,%rdi
    30bd:	call   2130 <omTensorGetStrides@plt>
    30c2:	mov    %r12,-0x60(%r15)
    30c6:	mov    %r12,-0x58(%r15)
    30ca:	movq   $0x0,-0x50(%r15)
    30d2:	vmovups 0x0(%r13),%ymm0
    30d8:	vmovups %ymm0,-0x48(%r15)
    30de:	vmovups (%rax),%ymm0
    30e2:	vmovups %ymm0,-0x28(%r15)
    30e8:	lea    -0x60(%rbx),%rdi
    30ec:	mov    -0x30(%rbp),%rsi
    30f0:	vzeroupper
    30f3:	call   2160 <_mlir_ciface_main_graph_model@plt>
    30f8:	mov    -0x60(%rbx),%r15
    30fc:	mov    -0x58(%rbx),%r12
    3100:	mov    -0x48(%rbx),%rax
    3104:	mov    %rax,-0x58(%rbp)
    3108:	mov    -0x40(%rbx),%rax
    310c:	mov    %rax,-0x60(%rbp)
    3110:	mov    -0x38(%rbx),%rax
    3114:	mov    %rax,-0x68(%rbp)
    3118:	mov    -0x30(%rbx),%rax
    311c:	mov    %rax,-0x30(%rbp)
    3120:	mov    -0x28(%rbx),%rax
    3124:	mov    %rax,-0x38(%rbp)
    3128:	mov    -0x20(%rbx),%rax
    312c:	mov    %rax,-0x40(%rbp)
    3130:	mov    -0x18(%rbx),%rax
    3134:	mov    %rax,-0x48(%rbp)
    3138:	mov    -0x10(%rbx),%rax
    313c:	mov    %rax,-0x50(%rbp)
    3140:	mov    %rsp,%r13
    3143:	lea    -0x10(%r13),%rbx
    3147:	mov    %rbx,%rsp
    314a:	mov    $0x4,%edi
    314f:	call   2290 <omTensorCreateUntyped@plt>
    3154:	mov    %rax,%r14
    3157:	mov    $0x1,%esi
    315c:	mov    %rax,%rdi
    315f:	mov    %r15,%rdx
    3162:	mov    %r12,%rcx
    3165:	call   20d0 <omTensorSetDataPtr@plt>
    316a:	mov    $0x1,%esi
    316f:	mov    %r14,%rdi
    3172:	call   22a0 <omTensorSetDataType@plt>
    3177:	mov    %r14,%rdi
    317a:	call   20e0 <omTensorGetShape@plt>
    317f:	mov    %rax,%r15
    3182:	mov    %r14,%rdi
    3185:	call   2130 <omTensorGetStrides@plt>
    318a:	mov    -0x58(%rbp),%rcx
    318e:	mov    %rcx,(%r15)
    3191:	mov    -0x38(%rbp),%rcx
    3195:	mov    %rcx,(%rax)
    3198:	mov    -0x60(%rbp),%rcx
    319c:	mov    %rcx,0x8(%r15)
    31a0:	mov    -0x40(%rbp),%rcx
    31a4:	mov    %rcx,0x8(%rax)
    31a8:	mov    -0x68(%rbp),%rcx
    31ac:	mov    %rcx,0x10(%r15)
    31b0:	mov    -0x48(%rbp),%rcx
    31b4:	mov    %rcx,0x10(%rax)
    31b8:	mov    -0x30(%rbp),%rcx
    31bc:	mov    %rcx,0x18(%r15)
    31c0:	mov    -0x50(%rbp),%rcx
    31c4:	mov    %rcx,0x18(%rax)
    31c8:	mov    %r14,-0x10(%r13)
    31cc:	mov    $0x1,%esi
    31d1:	mov    %rbx,%rdi
    31d4:	call   21d0 <omTensorListCreate@plt>
    31d9:	mov    %rax,%rbx
    31dc:	jmp    320e <run_main_graph_model+0x21e>
    31de:	lea    0x326b(%rip),%rdi        # 6450 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    31e5:	xor    %ebx,%ebx
    31e7:	mov    %rax,%rsi
    31ea:	xor    %eax,%eax
    31ec:	call   2040 <printf@plt>
    31f1:	jmp    3203 <run_main_graph_model+0x213>
    31f3:	lea    0x3226(%rip),%rdi        # 6420 <om_Wrong data type for the input 0: expect f32^J_model>
    31fa:	xor    %ebx,%ebx
    31fc:	xor    %eax,%eax
    31fe:	call   2040 <printf@plt>
    3203:	call   2030 <__errno_location@plt>
    3208:	movl   $0x16,(%rax)
    320e:	mov    %rbx,%rax
    3211:	lea    -0x28(%rbp),%rsp
    3215:	pop    %rbx
    3216:	pop    %r12
    3218:	pop    %r13
    321a:	pop    %r14
    321c:	pop    %r15
    321e:	pop    %rbp
    321f:	ret
    3220:	lea    0x31b9(%rip),%rdi        # 63e0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    3227:	jmp    31e5 <run_main_graph_model+0x1f5>
    3229:	lea    0x3160(%rip),%rdi        # 6390 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    3230:	jmp    324b <run_main_graph_model+0x25b>
    3232:	lea    0x3107(%rip),%rdi        # 6340 <om_Wrong size for the dimension 1 of the input 0: expect 64, but got %lld^J_model>
    3239:	jmp    324b <run_main_graph_model+0x25b>
    323b:	lea    0x30ae(%rip),%rdi        # 62f0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    3242:	jmp    324b <run_main_graph_model+0x25b>
    3244:	lea    0x3055(%rip),%rdi        # 62a0 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    324b:	xor    %ebx,%ebx
    324d:	jmp    31ea <run_main_graph_model+0x1fa>
    324f:	nop
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x6030>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
