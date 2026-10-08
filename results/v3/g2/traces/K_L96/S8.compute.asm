## _mlir_ciface_main_graph_model
    24f0:	push   %r14
    24f2:	push   %rbx
    24f3:	push   %rax
    24f4:	mov    %rdi,%rbx
    24f7:	mov    0x8(%rsi),%r14
    24fb:	mov    $0x48010,%edi
    2500:	call   21d0 <malloc@plt>
    2505:	mov    %rax,%rcx
    2508:	add    $0xf,%rax
    250c:	and    $0xfffffffffffffff0,%rax
    2510:	lea    0x1c(%rax),%rdx
    2514:	xor    %esi,%esi
    2516:	cs nopw 0x0(%rax,%rax,1)
    2520:	mov    %r14,%rdi
    2523:	mov    %rdx,%r8
    2526:	xor    %r9d,%r9d
    2529:	nopl   0x0(%rax)
    2530:	mov    $0xfffffffffffffff8,%r10
    2537:	mov    %rdi,%r11
    253a:	nopw   0x0(%rax,%rax,1)
    2540:	vmovss (%r11),%xmm0
    2545:	vmovss %xmm0,0x4(%r8,%r10,4)
    254c:	vmovss 0xc00(%r11),%xmm0
    2555:	vmovss %xmm0,0x8(%r8,%r10,4)
    255c:	vmovss 0x1800(%r11),%xmm0
    2565:	vmovss %xmm0,0xc(%r8,%r10,4)
    256c:	vmovss 0x2400(%r11),%xmm0
    2575:	vmovss %xmm0,0x10(%r8,%r10,4)
    257c:	vmovss 0x3000(%r11),%xmm0
    2585:	vmovss %xmm0,0x14(%r8,%r10,4)
    258c:	vmovss 0x3c00(%r11),%xmm0
    2595:	vmovss %xmm0,0x18(%r8,%r10,4)
    259c:	vmovss 0x4800(%r11),%xmm0
    25a5:	vmovss %xmm0,0x1c(%r8,%r10,4)
    25ac:	vmovss 0x5400(%r11),%xmm0
    25b5:	vmovss %xmm0,0x20(%r8,%r10,4)
    25bc:	add    $0x8,%r10
    25c0:	add    $0x6000,%r11
    25c7:	cmp    $0x58,%r10
    25cb:	jb     2540 <_mlir_ciface_main_graph_model+0x50>
    25d1:	inc    %r9
    25d4:	add    $0x180,%r8
    25db:	add    $0x4,%rdi
    25df:	cmp    $0x40,%r9
    25e3:	jne    2530 <_mlir_ciface_main_graph_model+0x40>
    25e9:	inc    %rsi
    25ec:	add    $0x6000,%rdx
    25f3:	add    $0x100,%r14
    25fa:	cmp    $0xc,%rsi
    25fe:	jne    2520 <_mlir_ciface_main_graph_model+0x30>
    2604:	mov    %rcx,(%rbx)
    2607:	mov    %rax,0x8(%rbx)
    260b:	vmovaps 0x3a2d(%rip),%ymm0        # 6040 <_fini+0xe5c>
    2613:	vmovups %ymm0,0x10(%rbx)
    2618:	vmovaps 0x3a40(%rip),%ymm0        # 6060 <_fini+0xe7c>
    2620:	vmovups %ymm0,0x30(%rbx)
    2625:	movq   $0x1,0x50(%rbx)
    262d:	add    $0x8,%rsp
    2631:	pop    %rbx
    2632:	pop    %r14
    2634:	vzeroupper
    2637:	ret
    2638:	nopl   0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2150:	jmp    *0x6f3a(%rip)        # 9090 <_mlir_ciface_main_graph_model@@Base+0x6ba0>
    2156:	push   $0x12
    215b:	jmp    2020 <_init+0x20>
## main_graph_model
    23a0:	push   %r14
    23a2:	push   %rbx
    23a3:	push   %rax
    23a4:	mov    %rdx,%r14
    23a7:	mov    %rdi,%rbx
    23aa:	mov    $0x48010,%edi
    23af:	call   21d0 <malloc@plt>
    23b4:	mov    %rax,%rcx
    23b7:	add    $0xf,%rax
    23bb:	and    $0xfffffffffffffff0,%rax
    23bf:	lea    0x1c(%rax),%rdx
    23c3:	xor    %esi,%esi
    23c5:	data16 cs nopw 0x0(%rax,%rax,1)
    23d0:	mov    %rdx,%rdi
    23d3:	mov    %r14,%r8
    23d6:	xor    %r9d,%r9d
    23d9:	nopl   0x0(%rax)
    23e0:	mov    $0xfffffffffffffff8,%r10
    23e7:	mov    %r8,%r11
    23ea:	nopw   0x0(%rax,%rax,1)
    23f0:	vmovss (%r11),%xmm0
    23f5:	vmovss %xmm0,0x4(%rdi,%r10,4)
    23fc:	vmovss 0xc00(%r11),%xmm0
    2405:	vmovss %xmm0,0x8(%rdi,%r10,4)
    240c:	vmovss 0x1800(%r11),%xmm0
    2415:	vmovss %xmm0,0xc(%rdi,%r10,4)
    241c:	vmovss 0x2400(%r11),%xmm0
    2425:	vmovss %xmm0,0x10(%rdi,%r10,4)
    242c:	vmovss 0x3000(%r11),%xmm0
    2435:	vmovss %xmm0,0x14(%rdi,%r10,4)
    243c:	vmovss 0x3c00(%r11),%xmm0
    2445:	vmovss %xmm0,0x18(%rdi,%r10,4)
    244c:	vmovss 0x4800(%r11),%xmm0
    2455:	vmovss %xmm0,0x1c(%rdi,%r10,4)
    245c:	vmovss 0x5400(%r11),%xmm0
    2465:	vmovss %xmm0,0x20(%rdi,%r10,4)
    246c:	add    $0x8,%r10
    2470:	add    $0x6000,%r11
    2477:	cmp    $0x58,%r10
    247b:	jb     23f0 <main_graph_model+0x50>
    2481:	inc    %r9
    2484:	add    $0x4,%r8
    2488:	add    $0x180,%rdi
    248f:	cmp    $0x40,%r9
    2493:	jne    23e0 <main_graph_model+0x40>
    2499:	inc    %rsi
    249c:	add    $0x100,%r14
    24a3:	add    $0x6000,%rdx
    24aa:	cmp    $0xc,%rsi
    24ae:	jne    23d0 <main_graph_model+0x30>
    24b4:	vmovaps 0x3b44(%rip),%ymm0        # 6000 <_fini+0xe1c>
    24bc:	vmovups %ymm0,0x38(%rbx)
    24c1:	vmovaps 0x3b57(%rip),%ymm0        # 6020 <_fini+0xe3c>
    24c9:	vmovups %ymm0,0x18(%rbx)
    24ce:	mov    %rax,0x8(%rbx)
    24d2:	mov    %rcx,(%rbx)
    24d5:	movq   $0x0,0x10(%rbx)
    24dd:	mov    %rbx,%rax
    24e0:	add    $0x8,%rsp
    24e4:	pop    %rbx
    24e5:	pop    %r14
    24e7:	vzeroupper
    24ea:	ret
    24eb:	nopl   0x0(%rax,%rax,1)
## run_main_graph
    28a0:	jmp    2070 <run_main_graph_model@plt>
    28a5:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    2640:	push   %rbp
    2641:	mov    %rsp,%rbp
    2644:	push   %r15
    2646:	push   %r14
    2648:	push   %r13
    264a:	push   %r12
    264c:	push   %rbx
    264d:	sub    $0x48,%rsp
    2651:	mov    %rdi,%rbx
    2654:	call   22a0 <omTensorListGetSize@plt>
    2659:	cmp    $0x1,%rax
    265d:	jne    282e <run_main_graph_model+0x1ee>
    2663:	mov    %rbx,%rdi
    2666:	call   2140 <omTensorListGetOmtArray@plt>
    266b:	mov    (%rax),%r14
    266e:	mov    %r14,%rdi
    2671:	call   22b0 <omTensorGetDataType@plt>
    2676:	cmp    $0x1,%rax
    267a:	jne    2843 <run_main_graph_model+0x203>
    2680:	mov    %r14,%rdi
    2683:	call   20f0 <omTensorGetRank@plt>
    2688:	cmp    $0x4,%rax
    268c:	jne    2870 <run_main_graph_model+0x230>
    2692:	mov    %r14,%rdi
    2695:	call   20e0 <omTensorGetShape@plt>
    269a:	mov    (%rax),%rsi
    269d:	cmp    $0x1,%rsi
    26a1:	jne    2879 <run_main_graph_model+0x239>
    26a7:	mov    0x8(%rax),%rsi
    26ab:	cmp    $0x60,%rsi
    26af:	jne    2882 <run_main_graph_model+0x242>
    26b5:	mov    0x10(%rax),%rsi
    26b9:	cmp    $0xc,%rsi
    26bd:	jne    288b <run_main_graph_model+0x24b>
    26c3:	mov    0x18(%rax),%rsi
    26c7:	cmp    $0x40,%rsi
    26cb:	jne    2894 <run_main_graph_model+0x254>
    26d1:	mov    %rbx,%rdi
    26d4:	call   2140 <omTensorListGetOmtArray@plt>
    26d9:	mov    %rsp,%rbx
    26dc:	lea    -0x60(%rbx),%rcx
    26e0:	mov    %rcx,%rsp
    26e3:	mov    (%rax),%r14
    26e6:	mov    %rsp,%r15
    26e9:	lea    -0x60(%r15),%rax
    26ed:	mov    %rax,-0x30(%rbp)
    26f1:	mov    %rax,%rsp
    26f4:	mov    %r14,%rdi
    26f7:	call   20a0 <omTensorGetDataPtr@plt>
    26fc:	mov    %rax,%r12
    26ff:	mov    %r14,%rdi
    2702:	call   20e0 <omTensorGetShape@plt>
    2707:	mov    %rax,%r13
    270a:	mov    %r14,%rdi
    270d:	call   2120 <omTensorGetStrides@plt>
    2712:	mov    %r12,-0x60(%r15)
    2716:	mov    %r12,-0x58(%r15)
    271a:	movq   $0x0,-0x50(%r15)
    2722:	vmovups 0x0(%r13),%ymm0
    2728:	vmovups %ymm0,-0x48(%r15)
    272e:	vmovups (%rax),%ymm0
    2732:	vmovups %ymm0,-0x28(%r15)
    2738:	lea    -0x60(%rbx),%rdi
    273c:	mov    -0x30(%rbp),%rsi
    2740:	vzeroupper
    2743:	call   2150 <_mlir_ciface_main_graph_model@plt>
    2748:	mov    -0x60(%rbx),%r15
    274c:	mov    -0x58(%rbx),%r12
    2750:	mov    -0x48(%rbx),%rax
    2754:	mov    %rax,-0x58(%rbp)
    2758:	mov    -0x40(%rbx),%rax
    275c:	mov    %rax,-0x60(%rbp)
    2760:	mov    -0x38(%rbx),%rax
    2764:	mov    %rax,-0x68(%rbp)
    2768:	mov    -0x30(%rbx),%rax
    276c:	mov    %rax,-0x30(%rbp)
    2770:	mov    -0x28(%rbx),%rax
    2774:	mov    %rax,-0x38(%rbp)
    2778:	mov    -0x20(%rbx),%rax
    277c:	mov    %rax,-0x40(%rbp)
    2780:	mov    -0x18(%rbx),%rax
    2784:	mov    %rax,-0x48(%rbp)
    2788:	mov    -0x10(%rbx),%rax
    278c:	mov    %rax,-0x50(%rbp)
    2790:	mov    %rsp,%r13
    2793:	lea    -0x10(%r13),%rbx
    2797:	mov    %rbx,%rsp
    279a:	mov    $0x4,%edi
    279f:	call   2280 <omTensorCreateUntyped@plt>
    27a4:	mov    %rax,%r14
    27a7:	mov    $0x1,%esi
    27ac:	mov    %rax,%rdi
    27af:	mov    %r15,%rdx
    27b2:	mov    %r12,%rcx
    27b5:	call   20d0 <omTensorSetDataPtr@plt>
    27ba:	mov    $0x1,%esi
    27bf:	mov    %r14,%rdi
    27c2:	call   2290 <omTensorSetDataType@plt>
    27c7:	mov    %r14,%rdi
    27ca:	call   20e0 <omTensorGetShape@plt>
    27cf:	mov    %rax,%r15
    27d2:	mov    %r14,%rdi
    27d5:	call   2120 <omTensorGetStrides@plt>
    27da:	mov    -0x58(%rbp),%rcx
    27de:	mov    %rcx,(%r15)
    27e1:	mov    -0x38(%rbp),%rcx
    27e5:	mov    %rcx,(%rax)
    27e8:	mov    -0x60(%rbp),%rcx
    27ec:	mov    %rcx,0x8(%r15)
    27f0:	mov    -0x40(%rbp),%rcx
    27f4:	mov    %rcx,0x8(%rax)
    27f8:	mov    -0x68(%rbp),%rcx
    27fc:	mov    %rcx,0x10(%r15)
    2800:	mov    -0x48(%rbp),%rcx
    2804:	mov    %rcx,0x10(%rax)
    2808:	mov    -0x30(%rbp),%rcx
    280c:	mov    %rcx,0x18(%r15)
    2810:	mov    -0x50(%rbp),%rcx
    2814:	mov    %rcx,0x18(%rax)
    2818:	mov    %r14,-0x10(%r13)
    281c:	mov    $0x1,%esi
    2821:	mov    %rbx,%rdi
    2824:	call   21c0 <omTensorListCreate@plt>
    2829:	mov    %rax,%rbx
    282c:	jmp    285e <run_main_graph_model+0x21e>
    282e:	lea    0x3adb(%rip),%rdi        # 6310 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    2835:	xor    %ebx,%ebx
    2837:	mov    %rax,%rsi
    283a:	xor    %eax,%eax
    283c:	call   2040 <printf@plt>
    2841:	jmp    2853 <run_main_graph_model+0x213>
    2843:	lea    0x3a96(%rip),%rdi        # 62e0 <om_Wrong data type for the input 0: expect f32^J_model>
    284a:	xor    %ebx,%ebx
    284c:	xor    %eax,%eax
    284e:	call   2040 <printf@plt>
    2853:	call   2030 <__errno_location@plt>
    2858:	movl   $0x16,(%rax)
    285e:	mov    %rbx,%rax
    2861:	lea    -0x28(%rbp),%rsp
    2865:	pop    %rbx
    2866:	pop    %r12
    2868:	pop    %r13
    286a:	pop    %r14
    286c:	pop    %r15
    286e:	pop    %rbp
    286f:	ret
    2870:	lea    0x3a29(%rip),%rdi        # 62a0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    2877:	jmp    2835 <run_main_graph_model+0x1f5>
    2879:	lea    0x39d0(%rip),%rdi        # 6250 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    2880:	jmp    289b <run_main_graph_model+0x25b>
    2882:	lea    0x3977(%rip),%rdi        # 6200 <om_Wrong size for the dimension 1 of the input 0: expect 96, but got %lld^J_model>
    2889:	jmp    289b <run_main_graph_model+0x25b>
    288b:	lea    0x391e(%rip),%rdi        # 61b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    2892:	jmp    289b <run_main_graph_model+0x25b>
    2894:	lea    0x38c5(%rip),%rdi        # 6160 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    289b:	xor    %ebx,%ebx
    289d:	jmp    283a <run_main_graph_model+0x1fa>
    289f:	nop
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x69e0>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
