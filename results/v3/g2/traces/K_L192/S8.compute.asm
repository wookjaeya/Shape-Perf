## _mlir_ciface_main_graph_model
    29f0:	push   %rbp
    29f1:	push   %r15
    29f3:	push   %r14
    29f5:	push   %r13
    29f7:	push   %r12
    29f9:	push   %rbx
    29fa:	push   %rax
    29fb:	mov    %rdi,%rbx
    29fe:	mov    0x8(%rsi),%r14
    2a02:	mov    $0x90010,%edi
    2a07:	call   21d0 <malloc@plt>
    2a0c:	mov    %rax,(%rsp)
    2a10:	add    $0xf,%rax
    2a14:	and    $0xfffffffffffffff0,%rax
    2a18:	lea    0x1c(%rax),%rdx
    2a1c:	xor    %esi,%esi
    2a1e:	vmovaps 0x3b58(%rip),%zmm8        # 6580 <_fini+0x98c>
    2a28:	vmovaps 0x3b8e(%rip),%zmm9        # 65c0 <_fini+0x9cc>
    2a32:	mov    $0xcc,%dil
    2a35:	vmovaps 0x3bc1(%rip),%zmm10        # 6600 <_fini+0xa0c>
    2a3f:	vmovaps 0x3bf7(%rip),%zmm11        # 6640 <_fini+0xa4c>
    2a49:	vmovaps 0x3c2d(%rip),%zmm12        # 6680 <_fini+0xa8c>
    2a53:	vmovaps 0x3c63(%rip),%zmm13        # 66c0 <_fini+0xacc>
    2a5d:	vmovaps 0x3c99(%rip),%zmm14        # 6700 <_fini+0xb0c>
    2a67:	vmovaps 0x3ccf(%rip),%zmm15        # 6740 <_fini+0xb4c>
    2a71:	vmovaps 0x36a5(%rip),%ymm17        # 6120 <_fini+0x52c>
    2a7b:	vmovaps 0x36bb(%rip),%ymm18        # 6140 <_fini+0x54c>
    2a85:	vmovaps 0x36d1(%rip),%ymm19        # 6160 <_fini+0x56c>
    2a8f:	vmovaps 0x36e7(%rip),%ymm20        # 6180 <_fini+0x58c>
    2a99:	vmovaps 0x36fd(%rip),%ymm21        # 61a0 <_fini+0x5ac>
    2aa3:	vmovaps 0x3713(%rip),%ymm22        # 61c0 <_fini+0x5cc>
    2aad:	vmovaps 0x3729(%rip),%ymm23        # 61e0 <_fini+0x5ec>
    2ab7:	vmovaps 0x373f(%rip),%ymm24        # 6200 <_fini+0x60c>
    2ac1:	vmovaps 0x3755(%rip),%ymm25        # 6220 <_fini+0x62c>
    2acb:	vmovaps 0x376b(%rip),%ymm26        # 6240 <_fini+0x64c>
    2ad5:	vmovaps 0x3781(%rip),%ymm27        # 6260 <_fini+0x66c>
    2adf:	vmovaps 0x3797(%rip),%ymm28        # 6280 <_fini+0x68c>
    2ae9:	vmovaps 0x37ad(%rip),%ymm29        # 62a0 <_fini+0x6ac>
    2af3:	vmovaps 0x37c3(%rip),%ymm30        # 62c0 <_fini+0x6cc>
    2afd:	vmovaps 0x37d9(%rip),%ymm31        # 62e0 <_fini+0x6ec>
    2b07:	mov    %r14,%r8
    2b0a:	vmovaps 0x35cc(%rip),%ymm16        # 60e0 <_fini+0x4ec>
    2b14:	jmp    2b3b <_mlir_ciface_main_graph_model+0x14b>
    2b16:	cs nopw 0x0(%rax,%rax,1)
    2b20:	inc    %rsi
    2b23:	add    $0xc000,%rdx
    2b2a:	add    $0x100,%r8
    2b31:	cmp    $0xc,%rsi
    2b35:	je     2ff2 <_mlir_ciface_main_graph_model+0x602>
    2b3b:	lea    (%rsi,%rsi,2),%rcx
    2b3f:	shl    $0xe,%rcx
    2b43:	lea    (%rax,%rcx,1),%r9
    2b47:	lea    0xc000(%rax,%rcx,1),%rcx
    2b4f:	mov    %rsi,%r10
    2b52:	shl    $0x8,%r10
    2b56:	lea    0x8f500(%r14,%r10,1),%r11
    2b5e:	cmp    %r11,%r9
    2b61:	setb   %bpl
    2b65:	add    %r14,%r10
    2b68:	cmp    %rcx,%r10
    2b6b:	setb   %r11b
    2b6f:	and    %bpl,%r11b
    2b72:	mov    %r8,%r15
    2b75:	mov    %rdx,%r12
    2b78:	xor    %r13d,%r13d
    2b7b:	jmp    2f4b <_mlir_ciface_main_graph_model+0x55b>
    2b80:	lea    (%r10,%r13,4),%rcx
    2b84:	kxnorb %k0,%k0,%k1
    2b88:	vxorps %xmm0,%xmm0,%xmm0
    2b8c:	vmovaps 0x346c(%rip),%ymm1        # 6000 <_fini+0x40c>
    2b94:	vgatherdps (%rcx,%ymm1,1),%ymm0{%k1}
    2b9b:	kxnorb %k0,%k0,%k1
    2b9f:	vxorps %xmm1,%xmm1,%xmm1
    2ba3:	vmovaps 0x3475(%rip),%ymm2        # 6020 <_fini+0x42c>
    2bab:	vgatherdps (%rcx,%ymm2,1),%ymm1{%k1}
    2bb2:	kxnorb %k0,%k0,%k1
    2bb6:	vxorps %xmm2,%xmm2,%xmm2
    2bba:	vmovaps 0x347e(%rip),%ymm3        # 6040 <_fini+0x44c>
    2bc2:	vgatherdps (%rcx,%ymm3,1),%ymm2{%k1}
    2bc9:	kxnorb %k0,%k0,%k1
    2bcd:	vxorps %xmm3,%xmm3,%xmm3
    2bd1:	vmovaps 0x3487(%rip),%ymm4        # 6060 <_fini+0x46c>
    2bd9:	vgatherdps (%rcx,%ymm4,1),%ymm3{%k1}
    2be0:	kxnorb %k0,%k0,%k1
    2be4:	vxorps %xmm4,%xmm4,%xmm4
    2be8:	vmovaps 0x3490(%rip),%ymm5        # 6080 <_fini+0x48c>
    2bf0:	vgatherdps (%rcx,%ymm5,1),%ymm4{%k1}
    2bf7:	kxnorb %k0,%k0,%k1
    2bfb:	vxorps %xmm5,%xmm5,%xmm5
    2bff:	vmovaps 0x3499(%rip),%ymm6        # 60a0 <_fini+0x4ac>
    2c07:	vgatherdps (%rcx,%ymm6,1),%ymm5{%k1}
    2c0e:	kxnorb %k0,%k0,%k1
    2c12:	vxorps %xmm6,%xmm6,%xmm6
    2c16:	vmovaps 0x34a2(%rip),%ymm7        # 60c0 <_fini+0x4cc>
    2c1e:	vgatherdps (%rcx,%ymm7,1),%ymm6{%k1}
    2c25:	kxnorb %k0,%k0,%k1
    2c29:	vxorps %xmm7,%xmm7,%xmm7
    2c2d:	vgatherdps (%rcx,%ymm16,1),%ymm7{%k1}
    2c34:	mov    %r13,%rbp
    2c37:	shl    $0x8,%rbp
    2c3b:	lea    0x0(%rbp,%rbp,2),%rbp
    2c40:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    2c47:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    2c4e:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2c55:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    2c5c:	vmovaps %zmm2,%zmm4
    2c62:	vpermt2ps %zmm3,%zmm8,%zmm4
    2c68:	vmovaps %zmm0,%zmm5
    2c6e:	vpermt2ps %zmm1,%zmm9,%zmm5
    2c74:	kmovd  %edi,%k1
    2c78:	vmovapd %zmm4,%zmm5{%k1}
    2c7e:	vmovaps %zmm2,%zmm4
    2c84:	vpermt2ps %zmm3,%zmm10,%zmm4
    2c8a:	vmovaps %zmm2,%zmm6
    2c90:	vpermt2ps %zmm3,%zmm12,%zmm6
    2c96:	vmovaps %zmm0,%zmm7
    2c9c:	vpermt2ps %zmm1,%zmm13,%zmm7
    2ca2:	vmovapd %zmm6,%zmm7{%k1}
    2ca8:	vmovaps %zmm0,%zmm6
    2cae:	vpermt2ps %zmm1,%zmm11,%zmm6
    2cb4:	vpermt2ps %zmm3,%zmm14,%zmm2
    2cba:	vpermt2ps %zmm1,%zmm15,%zmm0
    2cc0:	vmovapd %zmm2,%zmm0{%k1}
    2cc6:	vmovupd %zmm0,0xc0(%r9,%rbp,1)
    2cce:	vmovupd %zmm7,0x80(%r9,%rbp,1)
    2cd6:	vmovapd %zmm4,%zmm6{%k1}
    2cdc:	vmovupd %zmm6,0x40(%r9,%rbp,1)
    2ce4:	vmovupd %zmm5,(%r9,%rbp,1)
    2ceb:	kxnorb %k0,%k0,%k2
    2cef:	vxorpd %xmm0,%xmm0,%xmm0
    2cf3:	vmovaps 0x3405(%rip),%ymm1        # 6100 <_fini+0x50c>
    2cfb:	vgatherdps (%rcx,%ymm1,1),%ymm0{%k2}
    2d02:	kxnorb %k0,%k0,%k2
    2d06:	vxorps %xmm1,%xmm1,%xmm1
    2d0a:	vgatherdps (%rcx,%ymm17,1),%ymm1{%k2}
    2d11:	kxnorb %k0,%k0,%k2
    2d15:	vxorps %xmm2,%xmm2,%xmm2
    2d19:	vgatherdps (%rcx,%ymm18,1),%ymm2{%k2}
    2d20:	kxnorb %k0,%k0,%k2
    2d24:	vxorps %xmm3,%xmm3,%xmm3
    2d28:	vgatherdps (%rcx,%ymm19,1),%ymm3{%k2}
    2d2f:	kxnorb %k0,%k0,%k2
    2d33:	vxorps %xmm4,%xmm4,%xmm4
    2d37:	vgatherdps (%rcx,%ymm20,1),%ymm4{%k2}
    2d3e:	kxnorb %k0,%k0,%k2
    2d42:	vxorpd %xmm5,%xmm5,%xmm5
    2d46:	vgatherdps (%rcx,%ymm21,1),%ymm5{%k2}
    2d4d:	kxnorb %k0,%k0,%k2
    2d51:	vxorpd %xmm6,%xmm6,%xmm6
    2d55:	vgatherdps (%rcx,%ymm22,1),%ymm6{%k2}
    2d5c:	kxnorb %k0,%k0,%k2
    2d60:	vxorpd %xmm7,%xmm7,%xmm7
    2d64:	vgatherdps (%rcx,%ymm23,1),%ymm7{%k2}
    2d6b:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    2d72:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    2d79:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2d80:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    2d87:	vmovaps %zmm2,%zmm4
    2d8d:	vpermt2ps %zmm3,%zmm8,%zmm4
    2d93:	vmovaps %zmm0,%zmm5
    2d99:	vpermt2ps %zmm1,%zmm9,%zmm5
    2d9f:	vmovapd %zmm4,%zmm5{%k1}
    2da5:	vmovaps %zmm2,%zmm4
    2dab:	vpermt2ps %zmm3,%zmm10,%zmm4
    2db1:	vmovaps %zmm2,%zmm6
    2db7:	vpermt2ps %zmm3,%zmm12,%zmm6
    2dbd:	vmovaps %zmm0,%zmm7
    2dc3:	vpermt2ps %zmm1,%zmm13,%zmm7
    2dc9:	vmovapd %zmm6,%zmm7{%k1}
    2dcf:	vmovaps %zmm0,%zmm6
    2dd5:	vpermt2ps %zmm1,%zmm11,%zmm6
    2ddb:	vpermt2ps %zmm3,%zmm14,%zmm2
    2de1:	vpermt2ps %zmm1,%zmm15,%zmm0
    2de7:	vmovapd %zmm2,%zmm0{%k1}
    2ded:	vmovupd %zmm0,0x1c0(%r9,%rbp,1)
    2df5:	vmovupd %zmm7,0x180(%r9,%rbp,1)
    2dfd:	vmovapd %zmm4,%zmm6{%k1}
    2e03:	vmovupd %zmm6,0x140(%r9,%rbp,1)
    2e0b:	vmovupd %zmm5,0x100(%r9,%rbp,1)
    2e13:	kxnorb %k0,%k0,%k2
    2e17:	vxorpd %xmm0,%xmm0,%xmm0
    2e1b:	vgatherdps (%rcx,%ymm24,1),%ymm0{%k2}
    2e22:	kxnorb %k0,%k0,%k2
    2e26:	vxorps %xmm1,%xmm1,%xmm1
    2e2a:	vgatherdps (%rcx,%ymm25,1),%ymm1{%k2}
    2e31:	kxnorb %k0,%k0,%k2
    2e35:	vxorps %xmm2,%xmm2,%xmm2
    2e39:	vgatherdps (%rcx,%ymm26,1),%ymm2{%k2}
    2e40:	kxnorb %k0,%k0,%k2
    2e44:	vxorps %xmm3,%xmm3,%xmm3
    2e48:	vgatherdps (%rcx,%ymm27,1),%ymm3{%k2}
    2e4f:	kxnorb %k0,%k0,%k2
    2e53:	vxorps %xmm4,%xmm4,%xmm4
    2e57:	vgatherdps (%rcx,%ymm28,1),%ymm4{%k2}
    2e5e:	kxnorb %k0,%k0,%k2
    2e62:	vxorpd %xmm5,%xmm5,%xmm5
    2e66:	vgatherdps (%rcx,%ymm29,1),%ymm5{%k2}
    2e6d:	kxnorb %k0,%k0,%k2
    2e71:	vxorpd %xmm6,%xmm6,%xmm6
    2e75:	vgatherdps (%rcx,%ymm30,1),%ymm6{%k2}
    2e7c:	kxnorb %k0,%k0,%k2
    2e80:	vxorpd %xmm7,%xmm7,%xmm7
    2e84:	vgatherdps (%rcx,%ymm31,1),%ymm7{%k2}
    2e8b:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    2e92:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    2e99:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2ea0:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    2ea7:	vmovaps %zmm2,%zmm4
    2ead:	vpermt2ps %zmm3,%zmm8,%zmm4
    2eb3:	vmovaps %zmm0,%zmm5
    2eb9:	vpermt2ps %zmm1,%zmm9,%zmm5
    2ebf:	vmovapd %zmm4,%zmm5{%k1}
    2ec5:	vmovaps %zmm2,%zmm4
    2ecb:	vpermt2ps %zmm3,%zmm10,%zmm4
    2ed1:	vmovaps %zmm0,%zmm6
    2ed7:	vpermt2ps %zmm1,%zmm11,%zmm6
    2edd:	vmovapd %zmm4,%zmm6{%k1}
    2ee3:	vmovaps %zmm2,%zmm4
    2ee9:	vpermt2ps %zmm3,%zmm12,%zmm4
    2eef:	vmovaps %zmm0,%zmm7
    2ef5:	vpermt2ps %zmm1,%zmm13,%zmm7
    2efb:	vmovapd %zmm4,%zmm7{%k1}
    2f01:	vpermt2ps %zmm3,%zmm14,%zmm2
    2f07:	vpermt2ps %zmm1,%zmm15,%zmm0
    2f0d:	vmovapd %zmm2,%zmm0{%k1}
    2f13:	vmovupd %zmm0,0x2c0(%r9,%rbp,1)
    2f1b:	vmovupd %zmm7,0x280(%r9,%rbp,1)
    2f23:	vmovupd %zmm6,0x240(%r9,%rbp,1)
    2f2b:	vmovupd %zmm5,0x200(%r9,%rbp,1)
    2f33:	inc    %r13
    2f36:	add    $0x300,%r12
    2f3d:	add    $0x4,%r15
    2f41:	cmp    $0x40,%r13
    2f45:	je     2b20 <_mlir_ciface_main_graph_model+0x130>
    2f4b:	test   %r11b,%r11b
    2f4e:	je     2b80 <_mlir_ciface_main_graph_model+0x190>
    2f54:	mov    $0xfffffffffffffff8,%rcx
    2f5b:	mov    %r15,%rbp
    2f5e:	xchg   %ax,%ax
    2f60:	vmovss 0x0(%rbp),%xmm0
    2f65:	vmovss %xmm0,0x4(%r12,%rcx,4)
    2f6c:	vmovss 0xc00(%rbp),%xmm0
    2f74:	vmovss %xmm0,0x8(%r12,%rcx,4)
    2f7b:	vmovss 0x1800(%rbp),%xmm0
    2f83:	vmovss %xmm0,0xc(%r12,%rcx,4)
    2f8a:	vmovss 0x2400(%rbp),%xmm0
    2f92:	vmovss %xmm0,0x10(%r12,%rcx,4)
    2f99:	vmovss 0x3000(%rbp),%xmm0
    2fa1:	vmovss %xmm0,0x14(%r12,%rcx,4)
    2fa8:	vmovss 0x3c00(%rbp),%xmm0
    2fb0:	vmovss %xmm0,0x18(%r12,%rcx,4)
    2fb7:	vmovss 0x4800(%rbp),%xmm0
    2fbf:	vmovss %xmm0,0x1c(%r12,%rcx,4)
    2fc6:	vmovss 0x5400(%rbp),%xmm0
    2fce:	vmovss %xmm0,0x20(%r12,%rcx,4)
    2fd5:	add    $0x8,%rcx
    2fd9:	add    $0x6000,%rbp
    2fe0:	cmp    $0xb8,%rcx
    2fe7:	jb     2f60 <_mlir_ciface_main_graph_model+0x570>
    2fed:	jmp    2f33 <_mlir_ciface_main_graph_model+0x543>
    2ff2:	mov    (%rsp),%rcx
    2ff6:	mov    %rcx,(%rbx)
    2ff9:	mov    %rax,0x8(%rbx)
    2ffd:	vmovaps 0x333b(%rip),%ymm0        # 6340 <_fini+0x74c>
    3005:	vmovups %ymm0,0x10(%rbx)
    300a:	vmovaps 0x334e(%rip),%ymm0        # 6360 <_fini+0x76c>
    3012:	vmovups %ymm0,0x30(%rbx)
    3017:	movq   $0x1,0x50(%rbx)
    301f:	add    $0x8,%rsp
    3023:	pop    %rbx
    3024:	pop    %r12
    3026:	pop    %r13
    3028:	pop    %r14
    302a:	pop    %r15
    302c:	pop    %rbp
    302d:	vzeroupper
    3030:	ret
    3031:	data16 data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## _mlir_ciface_main_graph_model@plt
    2150:	jmp    *0x6f3a(%rip)        # 9090 <_mlir_ciface_main_graph_model@@Base+0x66a0>
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
    23b1:	mov    $0x90010,%edi
    23b6:	call   21d0 <malloc@plt>
    23bb:	mov    %rax,(%rsp)
    23bf:	add    $0xf,%rax
    23c3:	and    $0xfffffffffffffff0,%rax
    23c7:	lea    0x1c(%rax),%rdx
    23cb:	xor    %esi,%esi
    23cd:	vmovaps 0x3fa9(%rip),%zmm8        # 6380 <_fini+0x78c>
    23d7:	vmovaps 0x3fdf(%rip),%zmm9        # 63c0 <_fini+0x7cc>
    23e1:	mov    $0xcc,%dil
    23e4:	vmovaps 0x4012(%rip),%zmm10        # 6400 <_fini+0x80c>
    23ee:	vmovaps 0x4048(%rip),%zmm11        # 6440 <_fini+0x84c>
    23f8:	vmovaps 0x407e(%rip),%zmm12        # 6480 <_fini+0x88c>
    2402:	vmovaps 0x40b4(%rip),%zmm13        # 64c0 <_fini+0x8cc>
    240c:	vmovaps 0x40ea(%rip),%zmm14        # 6500 <_fini+0x90c>
    2416:	vmovaps 0x4120(%rip),%zmm15        # 6540 <_fini+0x94c>
    2420:	vmovaps 0x3cf6(%rip),%ymm17        # 6120 <_fini+0x52c>
    242a:	vmovaps 0x3d0c(%rip),%ymm18        # 6140 <_fini+0x54c>
    2434:	vmovaps 0x3d22(%rip),%ymm19        # 6160 <_fini+0x56c>
    243e:	vmovaps 0x3d38(%rip),%ymm20        # 6180 <_fini+0x58c>
    2448:	vmovaps 0x3d4e(%rip),%ymm21        # 61a0 <_fini+0x5ac>
    2452:	vmovaps 0x3d64(%rip),%ymm22        # 61c0 <_fini+0x5cc>
    245c:	vmovaps 0x3d7a(%rip),%ymm23        # 61e0 <_fini+0x5ec>
    2466:	vmovaps 0x3d90(%rip),%ymm24        # 6200 <_fini+0x60c>
    2470:	vmovaps 0x3da6(%rip),%ymm25        # 6220 <_fini+0x62c>
    247a:	vmovaps 0x3dbc(%rip),%ymm26        # 6240 <_fini+0x64c>
    2484:	vmovaps 0x3dd2(%rip),%ymm27        # 6260 <_fini+0x66c>
    248e:	vmovaps 0x3de8(%rip),%ymm28        # 6280 <_fini+0x68c>
    2498:	vmovaps 0x3dfe(%rip),%ymm29        # 62a0 <_fini+0x6ac>
    24a2:	vmovaps 0x3e14(%rip),%ymm30        # 62c0 <_fini+0x6cc>
    24ac:	vmovaps 0x3e2a(%rip),%ymm31        # 62e0 <_fini+0x6ec>
    24b6:	mov    %r14,%r8
    24b9:	vmovaps 0x3c1d(%rip),%ymm16        # 60e0 <_fini+0x4ec>
    24c3:	jmp    24eb <main_graph_model+0x14b>
    24c5:	data16 cs nopw 0x0(%rax,%rax,1)
    24d0:	inc    %rsi
    24d3:	add    $0x100,%r8
    24da:	add    $0xc000,%rdx
    24e1:	cmp    $0xc,%rsi
    24e5:	je     29a2 <main_graph_model+0x602>
    24eb:	lea    (%rsi,%rsi,2),%rcx
    24ef:	shl    $0xe,%rcx
    24f3:	lea    (%rax,%rcx,1),%r9
    24f7:	lea    0xc000(%rax,%rcx,1),%rcx
    24ff:	mov    %rsi,%r10
    2502:	shl    $0x8,%r10
    2506:	lea    0x8f500(%r14,%r10,1),%r11
    250e:	cmp    %r11,%r9
    2511:	setb   %bpl
    2515:	add    %r14,%r10
    2518:	cmp    %rcx,%r10
    251b:	setb   %r11b
    251f:	and    %bpl,%r11b
    2522:	mov    %rdx,%r15
    2525:	mov    %r8,%r12
    2528:	xor    %r13d,%r13d
    252b:	jmp    28fb <main_graph_model+0x55b>
    2530:	lea    (%r10,%r13,4),%rcx
    2534:	kxnorb %k0,%k0,%k1
    2538:	vxorps %xmm0,%xmm0,%xmm0
    253c:	vmovaps 0x3abc(%rip),%ymm1        # 6000 <_fini+0x40c>
    2544:	vgatherdps (%rcx,%ymm1,1),%ymm0{%k1}
    254b:	kxnorb %k0,%k0,%k1
    254f:	vxorps %xmm1,%xmm1,%xmm1
    2553:	vmovaps 0x3ac5(%rip),%ymm2        # 6020 <_fini+0x42c>
    255b:	vgatherdps (%rcx,%ymm2,1),%ymm1{%k1}
    2562:	kxnorb %k0,%k0,%k1
    2566:	vxorps %xmm2,%xmm2,%xmm2
    256a:	vmovaps 0x3ace(%rip),%ymm3        # 6040 <_fini+0x44c>
    2572:	vgatherdps (%rcx,%ymm3,1),%ymm2{%k1}
    2579:	kxnorb %k0,%k0,%k1
    257d:	vxorps %xmm3,%xmm3,%xmm3
    2581:	vmovaps 0x3ad7(%rip),%ymm4        # 6060 <_fini+0x46c>
    2589:	vgatherdps (%rcx,%ymm4,1),%ymm3{%k1}
    2590:	kxnorb %k0,%k0,%k1
    2594:	vxorps %xmm4,%xmm4,%xmm4
    2598:	vmovaps 0x3ae0(%rip),%ymm5        # 6080 <_fini+0x48c>
    25a0:	vgatherdps (%rcx,%ymm5,1),%ymm4{%k1}
    25a7:	kxnorb %k0,%k0,%k1
    25ab:	vxorps %xmm5,%xmm5,%xmm5
    25af:	vmovaps 0x3ae9(%rip),%ymm6        # 60a0 <_fini+0x4ac>
    25b7:	vgatherdps (%rcx,%ymm6,1),%ymm5{%k1}
    25be:	kxnorb %k0,%k0,%k1
    25c2:	vxorps %xmm6,%xmm6,%xmm6
    25c6:	vmovaps 0x3af2(%rip),%ymm7        # 60c0 <_fini+0x4cc>
    25ce:	vgatherdps (%rcx,%ymm7,1),%ymm6{%k1}
    25d5:	kxnorb %k0,%k0,%k1
    25d9:	vxorps %xmm7,%xmm7,%xmm7
    25dd:	vgatherdps (%rcx,%ymm16,1),%ymm7{%k1}
    25e4:	mov    %r13,%rbp
    25e7:	shl    $0x8,%rbp
    25eb:	lea    0x0(%rbp,%rbp,2),%rbp
    25f0:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    25f7:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    25fe:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2605:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    260c:	vmovaps %zmm2,%zmm4
    2612:	vpermt2ps %zmm3,%zmm8,%zmm4
    2618:	vmovaps %zmm0,%zmm5
    261e:	vpermt2ps %zmm1,%zmm9,%zmm5
    2624:	kmovd  %edi,%k1
    2628:	vmovapd %zmm4,%zmm5{%k1}
    262e:	vmovaps %zmm2,%zmm4
    2634:	vpermt2ps %zmm3,%zmm10,%zmm4
    263a:	vmovaps %zmm2,%zmm6
    2640:	vpermt2ps %zmm3,%zmm12,%zmm6
    2646:	vmovaps %zmm0,%zmm7
    264c:	vpermt2ps %zmm1,%zmm13,%zmm7
    2652:	vmovapd %zmm6,%zmm7{%k1}
    2658:	vmovaps %zmm0,%zmm6
    265e:	vpermt2ps %zmm1,%zmm11,%zmm6
    2664:	vpermt2ps %zmm3,%zmm14,%zmm2
    266a:	vpermt2ps %zmm1,%zmm15,%zmm0
    2670:	vmovapd %zmm2,%zmm0{%k1}
    2676:	vmovupd %zmm0,0xc0(%r9,%rbp,1)
    267e:	vmovupd %zmm7,0x80(%r9,%rbp,1)
    2686:	vmovapd %zmm4,%zmm6{%k1}
    268c:	vmovupd %zmm6,0x40(%r9,%rbp,1)
    2694:	vmovupd %zmm5,(%r9,%rbp,1)
    269b:	kxnorb %k0,%k0,%k2
    269f:	vxorpd %xmm0,%xmm0,%xmm0
    26a3:	vmovaps 0x3a55(%rip),%ymm1        # 6100 <_fini+0x50c>
    26ab:	vgatherdps (%rcx,%ymm1,1),%ymm0{%k2}
    26b2:	kxnorb %k0,%k0,%k2
    26b6:	vxorps %xmm1,%xmm1,%xmm1
    26ba:	vgatherdps (%rcx,%ymm17,1),%ymm1{%k2}
    26c1:	kxnorb %k0,%k0,%k2
    26c5:	vxorps %xmm2,%xmm2,%xmm2
    26c9:	vgatherdps (%rcx,%ymm18,1),%ymm2{%k2}
    26d0:	kxnorb %k0,%k0,%k2
    26d4:	vxorps %xmm3,%xmm3,%xmm3
    26d8:	vgatherdps (%rcx,%ymm19,1),%ymm3{%k2}
    26df:	kxnorb %k0,%k0,%k2
    26e3:	vxorps %xmm4,%xmm4,%xmm4
    26e7:	vgatherdps (%rcx,%ymm20,1),%ymm4{%k2}
    26ee:	kxnorb %k0,%k0,%k2
    26f2:	vxorpd %xmm5,%xmm5,%xmm5
    26f6:	vgatherdps (%rcx,%ymm21,1),%ymm5{%k2}
    26fd:	kxnorb %k0,%k0,%k2
    2701:	vxorpd %xmm6,%xmm6,%xmm6
    2705:	vgatherdps (%rcx,%ymm22,1),%ymm6{%k2}
    270c:	kxnorb %k0,%k0,%k2
    2710:	vxorpd %xmm7,%xmm7,%xmm7
    2714:	vgatherdps (%rcx,%ymm23,1),%ymm7{%k2}
    271b:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    2722:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    2729:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2730:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    2737:	vmovaps %zmm2,%zmm4
    273d:	vpermt2ps %zmm3,%zmm8,%zmm4
    2743:	vmovaps %zmm0,%zmm5
    2749:	vpermt2ps %zmm1,%zmm9,%zmm5
    274f:	vmovapd %zmm4,%zmm5{%k1}
    2755:	vmovaps %zmm2,%zmm4
    275b:	vpermt2ps %zmm3,%zmm10,%zmm4
    2761:	vmovaps %zmm2,%zmm6
    2767:	vpermt2ps %zmm3,%zmm12,%zmm6
    276d:	vmovaps %zmm0,%zmm7
    2773:	vpermt2ps %zmm1,%zmm13,%zmm7
    2779:	vmovapd %zmm6,%zmm7{%k1}
    277f:	vmovaps %zmm0,%zmm6
    2785:	vpermt2ps %zmm1,%zmm11,%zmm6
    278b:	vpermt2ps %zmm3,%zmm14,%zmm2
    2791:	vpermt2ps %zmm1,%zmm15,%zmm0
    2797:	vmovapd %zmm2,%zmm0{%k1}
    279d:	vmovupd %zmm0,0x1c0(%r9,%rbp,1)
    27a5:	vmovupd %zmm7,0x180(%r9,%rbp,1)
    27ad:	vmovapd %zmm4,%zmm6{%k1}
    27b3:	vmovupd %zmm6,0x140(%r9,%rbp,1)
    27bb:	vmovupd %zmm5,0x100(%r9,%rbp,1)
    27c3:	kxnorb %k0,%k0,%k2
    27c7:	vxorpd %xmm0,%xmm0,%xmm0
    27cb:	vgatherdps (%rcx,%ymm24,1),%ymm0{%k2}
    27d2:	kxnorb %k0,%k0,%k2
    27d6:	vxorps %xmm1,%xmm1,%xmm1
    27da:	vgatherdps (%rcx,%ymm25,1),%ymm1{%k2}
    27e1:	kxnorb %k0,%k0,%k2
    27e5:	vxorps %xmm2,%xmm2,%xmm2
    27e9:	vgatherdps (%rcx,%ymm26,1),%ymm2{%k2}
    27f0:	kxnorb %k0,%k0,%k2
    27f4:	vxorps %xmm3,%xmm3,%xmm3
    27f8:	vgatherdps (%rcx,%ymm27,1),%ymm3{%k2}
    27ff:	kxnorb %k0,%k0,%k2
    2803:	vxorps %xmm4,%xmm4,%xmm4
    2807:	vgatherdps (%rcx,%ymm28,1),%ymm4{%k2}
    280e:	kxnorb %k0,%k0,%k2
    2812:	vxorpd %xmm5,%xmm5,%xmm5
    2816:	vgatherdps (%rcx,%ymm29,1),%ymm5{%k2}
    281d:	kxnorb %k0,%k0,%k2
    2821:	vxorpd %xmm6,%xmm6,%xmm6
    2825:	vgatherdps (%rcx,%ymm30,1),%ymm6{%k2}
    282c:	kxnorb %k0,%k0,%k2
    2830:	vxorpd %xmm7,%xmm7,%xmm7
    2834:	vgatherdps (%rcx,%ymm31,1),%ymm7{%k2}
    283b:	vinsertf64x4 $0x1,%ymm1,%zmm0,%zmm0
    2842:	vinsertf64x4 $0x1,%ymm3,%zmm2,%zmm1
    2849:	vinsertf64x4 $0x1,%ymm5,%zmm4,%zmm2
    2850:	vinsertf64x4 $0x1,%ymm7,%zmm6,%zmm3
    2857:	vmovaps %zmm2,%zmm4
    285d:	vpermt2ps %zmm3,%zmm8,%zmm4
    2863:	vmovaps %zmm0,%zmm5
    2869:	vpermt2ps %zmm1,%zmm9,%zmm5
    286f:	vmovapd %zmm4,%zmm5{%k1}
    2875:	vmovaps %zmm2,%zmm4
    287b:	vpermt2ps %zmm3,%zmm10,%zmm4
    2881:	vmovaps %zmm0,%zmm6
    2887:	vpermt2ps %zmm1,%zmm11,%zmm6
    288d:	vmovapd %zmm4,%zmm6{%k1}
    2893:	vmovaps %zmm2,%zmm4
    2899:	vpermt2ps %zmm3,%zmm12,%zmm4
    289f:	vmovaps %zmm0,%zmm7
    28a5:	vpermt2ps %zmm1,%zmm13,%zmm7
    28ab:	vmovapd %zmm4,%zmm7{%k1}
    28b1:	vpermt2ps %zmm3,%zmm14,%zmm2
    28b7:	vpermt2ps %zmm1,%zmm15,%zmm0
    28bd:	vmovapd %zmm2,%zmm0{%k1}
    28c3:	vmovupd %zmm0,0x2c0(%r9,%rbp,1)
    28cb:	vmovupd %zmm7,0x280(%r9,%rbp,1)
    28d3:	vmovupd %zmm6,0x240(%r9,%rbp,1)
    28db:	vmovupd %zmm5,0x200(%r9,%rbp,1)
    28e3:	inc    %r13
    28e6:	add    $0x4,%r12
    28ea:	add    $0x300,%r15
    28f1:	cmp    $0x40,%r13
    28f5:	je     24d0 <main_graph_model+0x130>
    28fb:	test   %r11b,%r11b
    28fe:	je     2530 <main_graph_model+0x190>
    2904:	mov    $0xfffffffffffffff8,%rcx
    290b:	mov    %r12,%rbp
    290e:	xchg   %ax,%ax
    2910:	vmovss 0x0(%rbp),%xmm0
    2915:	vmovss %xmm0,0x4(%r15,%rcx,4)
    291c:	vmovss 0xc00(%rbp),%xmm0
    2924:	vmovss %xmm0,0x8(%r15,%rcx,4)
    292b:	vmovss 0x1800(%rbp),%xmm0
    2933:	vmovss %xmm0,0xc(%r15,%rcx,4)
    293a:	vmovss 0x2400(%rbp),%xmm0
    2942:	vmovss %xmm0,0x10(%r15,%rcx,4)
    2949:	vmovss 0x3000(%rbp),%xmm0
    2951:	vmovss %xmm0,0x14(%r15,%rcx,4)
    2958:	vmovss 0x3c00(%rbp),%xmm0
    2960:	vmovss %xmm0,0x18(%r15,%rcx,4)
    2967:	vmovss 0x4800(%rbp),%xmm0
    296f:	vmovss %xmm0,0x1c(%r15,%rcx,4)
    2976:	vmovss 0x5400(%rbp),%xmm0
    297e:	vmovss %xmm0,0x20(%r15,%rcx,4)
    2985:	add    $0x8,%rcx
    2989:	add    $0x6000,%rbp
    2990:	cmp    $0xb8,%rcx
    2997:	jb     2910 <main_graph_model+0x570>
    299d:	jmp    28e3 <main_graph_model+0x543>
    29a2:	vmovaps 0x3956(%rip),%ymm0        # 6300 <_fini+0x70c>
    29aa:	vmovups %ymm0,0x38(%rbx)
    29af:	vmovaps 0x3969(%rip),%ymm0        # 6320 <_fini+0x72c>
    29b7:	vmovups %ymm0,0x18(%rbx)
    29bc:	mov    %rax,0x8(%rbx)
    29c0:	mov    (%rsp),%rax
    29c4:	mov    %rax,(%rbx)
    29c7:	movq   $0x0,0x10(%rbx)
    29cf:	mov    %rbx,%rax
    29d2:	add    $0x8,%rsp
    29d6:	pop    %rbx
    29d7:	pop    %r12
    29d9:	pop    %r13
    29db:	pop    %r14
    29dd:	pop    %r15
    29df:	pop    %rbp
    29e0:	vzeroupper
    29e3:	ret
    29e4:	data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph
    32b0:	jmp    2070 <run_main_graph_model@plt>
    32b5:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    3040:	push   %rbp
    3041:	mov    %rsp,%rbp
    3044:	push   %r15
    3046:	push   %r14
    3048:	push   %r13
    304a:	push   %r12
    304c:	push   %rbx
    304d:	sub    $0x48,%rsp
    3051:	mov    %rdi,%rbx
    3054:	call   22a0 <omTensorListGetSize@plt>
    3059:	cmp    $0x1,%rax
    305d:	jne    3231 <run_main_graph_model+0x1f1>
    3063:	mov    %rbx,%rdi
    3066:	call   2140 <omTensorListGetOmtArray@plt>
    306b:	mov    (%rax),%r14
    306e:	mov    %r14,%rdi
    3071:	call   22b0 <omTensorGetDataType@plt>
    3076:	cmp    $0x1,%rax
    307a:	jne    3246 <run_main_graph_model+0x206>
    3080:	mov    %r14,%rdi
    3083:	call   20f0 <omTensorGetRank@plt>
    3088:	cmp    $0x4,%rax
    308c:	jne    3273 <run_main_graph_model+0x233>
    3092:	mov    %r14,%rdi
    3095:	call   20e0 <omTensorGetShape@plt>
    309a:	mov    (%rax),%rsi
    309d:	cmp    $0x1,%rsi
    30a1:	jne    327c <run_main_graph_model+0x23c>
    30a7:	mov    0x8(%rax),%rsi
    30ab:	cmp    $0xc0,%rsi
    30b2:	jne    3285 <run_main_graph_model+0x245>
    30b8:	mov    0x10(%rax),%rsi
    30bc:	cmp    $0xc,%rsi
    30c0:	jne    328e <run_main_graph_model+0x24e>
    30c6:	mov    0x18(%rax),%rsi
    30ca:	cmp    $0x40,%rsi
    30ce:	jne    3297 <run_main_graph_model+0x257>
    30d4:	mov    %rbx,%rdi
    30d7:	call   2140 <omTensorListGetOmtArray@plt>
    30dc:	mov    %rsp,%rbx
    30df:	lea    -0x60(%rbx),%rcx
    30e3:	mov    %rcx,%rsp
    30e6:	mov    (%rax),%r14
    30e9:	mov    %rsp,%r15
    30ec:	lea    -0x60(%r15),%rax
    30f0:	mov    %rax,-0x30(%rbp)
    30f4:	mov    %rax,%rsp
    30f7:	mov    %r14,%rdi
    30fa:	call   20a0 <omTensorGetDataPtr@plt>
    30ff:	mov    %rax,%r12
    3102:	mov    %r14,%rdi
    3105:	call   20e0 <omTensorGetShape@plt>
    310a:	mov    %rax,%r13
    310d:	mov    %r14,%rdi
    3110:	call   2120 <omTensorGetStrides@plt>
    3115:	mov    %r12,-0x60(%r15)
    3119:	mov    %r12,-0x58(%r15)
    311d:	movq   $0x0,-0x50(%r15)
    3125:	vmovups 0x0(%r13),%ymm0
    312b:	vmovups %ymm0,-0x48(%r15)
    3131:	vmovups (%rax),%ymm0
    3135:	vmovups %ymm0,-0x28(%r15)
    313b:	lea    -0x60(%rbx),%rdi
    313f:	mov    -0x30(%rbp),%rsi
    3143:	vzeroupper
    3146:	call   2150 <_mlir_ciface_main_graph_model@plt>
    314b:	mov    -0x60(%rbx),%r15
    314f:	mov    -0x58(%rbx),%r12
    3153:	mov    -0x48(%rbx),%rax
    3157:	mov    %rax,-0x58(%rbp)
    315b:	mov    -0x40(%rbx),%rax
    315f:	mov    %rax,-0x60(%rbp)
    3163:	mov    -0x38(%rbx),%rax
    3167:	mov    %rax,-0x68(%rbp)
    316b:	mov    -0x30(%rbx),%rax
    316f:	mov    %rax,-0x30(%rbp)
    3173:	mov    -0x28(%rbx),%rax
    3177:	mov    %rax,-0x38(%rbp)
    317b:	mov    -0x20(%rbx),%rax
    317f:	mov    %rax,-0x40(%rbp)
    3183:	mov    -0x18(%rbx),%rax
    3187:	mov    %rax,-0x48(%rbp)
    318b:	mov    -0x10(%rbx),%rax
    318f:	mov    %rax,-0x50(%rbp)
    3193:	mov    %rsp,%r13
    3196:	lea    -0x10(%r13),%rbx
    319a:	mov    %rbx,%rsp
    319d:	mov    $0x4,%edi
    31a2:	call   2280 <omTensorCreateUntyped@plt>
    31a7:	mov    %rax,%r14
    31aa:	mov    $0x1,%esi
    31af:	mov    %rax,%rdi
    31b2:	mov    %r15,%rdx
    31b5:	mov    %r12,%rcx
    31b8:	call   20d0 <omTensorSetDataPtr@plt>
    31bd:	mov    $0x1,%esi
    31c2:	mov    %r14,%rdi
    31c5:	call   2290 <omTensorSetDataType@plt>
    31ca:	mov    %r14,%rdi
    31cd:	call   20e0 <omTensorGetShape@plt>
    31d2:	mov    %rax,%r15
    31d5:	mov    %r14,%rdi
    31d8:	call   2120 <omTensorGetStrides@plt>
    31dd:	mov    -0x58(%rbp),%rcx
    31e1:	mov    %rcx,(%r15)
    31e4:	mov    -0x38(%rbp),%rcx
    31e8:	mov    %rcx,(%rax)
    31eb:	mov    -0x60(%rbp),%rcx
    31ef:	mov    %rcx,0x8(%r15)
    31f3:	mov    -0x40(%rbp),%rcx
    31f7:	mov    %rcx,0x8(%rax)
    31fb:	mov    -0x68(%rbp),%rcx
    31ff:	mov    %rcx,0x10(%r15)
    3203:	mov    -0x48(%rbp),%rcx
    3207:	mov    %rcx,0x10(%rax)
    320b:	mov    -0x30(%rbp),%rcx
    320f:	mov    %rcx,0x18(%r15)
    3213:	mov    -0x50(%rbp),%rcx
    3217:	mov    %rcx,0x18(%rax)
    321b:	mov    %r14,-0x10(%r13)
    321f:	mov    $0x1,%esi
    3224:	mov    %rbx,%rdi
    3227:	call   21c0 <omTensorListCreate@plt>
    322c:	mov    %rax,%rbx
    322f:	jmp    3261 <run_main_graph_model+0x221>
    3231:	lea    0x37d8(%rip),%rdi        # 6a10 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    3238:	xor    %ebx,%ebx
    323a:	mov    %rax,%rsi
    323d:	xor    %eax,%eax
    323f:	call   2040 <printf@plt>
    3244:	jmp    3256 <run_main_graph_model+0x216>
    3246:	lea    0x3793(%rip),%rdi        # 69e0 <om_Wrong data type for the input 0: expect f32^J_model>
    324d:	xor    %ebx,%ebx
    324f:	xor    %eax,%eax
    3251:	call   2040 <printf@plt>
    3256:	call   2030 <__errno_location@plt>
    325b:	movl   $0x16,(%rax)
    3261:	mov    %rbx,%rax
    3264:	lea    -0x28(%rbp),%rsp
    3268:	pop    %rbx
    3269:	pop    %r12
    326b:	pop    %r13
    326d:	pop    %r14
    326f:	pop    %r15
    3271:	pop    %rbp
    3272:	ret
    3273:	lea    0x3726(%rip),%rdi        # 69a0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    327a:	jmp    3238 <run_main_graph_model+0x1f8>
    327c:	lea    0x36cd(%rip),%rdi        # 6950 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    3283:	jmp    329e <run_main_graph_model+0x25e>
    3285:	lea    0x3674(%rip),%rdi        # 6900 <om_Wrong size for the dimension 1 of the input 0: expect 192, but got %lld^J_model>
    328c:	jmp    329e <run_main_graph_model+0x25e>
    328e:	lea    0x361b(%rip),%rdi        # 68b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    3295:	jmp    329e <run_main_graph_model+0x25e>
    3297:	lea    0x35c2(%rip),%rdi        # 6860 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    329e:	xor    %ebx,%ebx
    32a0:	jmp    323d <run_main_graph_model+0x1fd>
    32a2:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x5fe0>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
