## _mlir_ciface_main_graph_model
    2bb0:	push   %rbp
    2bb1:	push   %r15
    2bb3:	push   %r14
    2bb5:	push   %r13
    2bb7:	push   %r12
    2bb9:	push   %rbx
    2bba:	push   %rax
    2bbb:	mov    %rdi,%rbx
    2bbe:	mov    0x8(%rsi),%r14
    2bc2:	mov    $0xc0010,%edi
    2bc7:	call   21d0 <malloc@plt>
    2bcc:	mov    %rax,(%rsp)
    2bd0:	add    $0xf,%rax
    2bd4:	and    $0xfffffffffffffff0,%rax
    2bd8:	lea    0x1c(%rax),%rdx
    2bdc:	xor    %esi,%esi
    2bde:	vmovaps 0x3a98(%rip),%zmm8        # 6680 <_fini+0x70c>
    2be8:	vmovaps 0x3ace(%rip),%zmm9        # 66c0 <_fini+0x74c>
    2bf2:	mov    $0xcc,%dil
    2bf5:	vmovaps 0x3b01(%rip),%zmm10        # 6700 <_fini+0x78c>
    2bff:	vmovaps 0x3b37(%rip),%zmm11        # 6740 <_fini+0x7cc>
    2c09:	vmovaps 0x3b6d(%rip),%zmm12        # 6780 <_fini+0x80c>
    2c13:	vmovaps 0x3ba3(%rip),%zmm13        # 67c0 <_fini+0x84c>
    2c1d:	vmovaps 0x3bd9(%rip),%zmm14        # 6800 <_fini+0x88c>
    2c27:	vmovaps 0x3c0f(%rip),%zmm15        # 6840 <_fini+0x8cc>
    2c31:	vmovaps 0x35e5(%rip),%ymm25        # 6220 <_fini+0x2ac>
    2c3b:	vmovaps 0x35fb(%rip),%ymm26        # 6240 <_fini+0x2cc>
    2c45:	vmovaps 0x3611(%rip),%ymm27        # 6260 <_fini+0x2ec>
    2c4f:	vmovaps 0x3627(%rip),%ymm28        # 6280 <_fini+0x30c>
    2c59:	vmovaps 0x363d(%rip),%ymm29        # 62a0 <_fini+0x32c>
    2c63:	vmovaps 0x3653(%rip),%ymm30        # 62c0 <_fini+0x34c>
    2c6d:	vmovaps 0x3669(%rip),%ymm31        # 62e0 <_fini+0x36c>
    2c77:	mov    %r14,%r8
    2c7a:	vmovaps 0x367e(%rip),%ymm0        # 6300 <_fini+0x38c>
    2c82:	vmovaps 0x3696(%rip),%ymm1        # 6320 <_fini+0x3ac>
    2c8a:	vmovaps 0x36ae(%rip),%ymm2        # 6340 <_fini+0x3cc>
    2c92:	vmovaps 0x36c6(%rip),%ymm3        # 6360 <_fini+0x3ec>
    2c9a:	vmovaps 0x36de(%rip),%ymm4        # 6380 <_fini+0x40c>
    2ca2:	vmovaps 0x36f6(%rip),%ymm5        # 63a0 <_fini+0x42c>
    2caa:	vmovaps 0x370e(%rip),%ymm6        # 63c0 <_fini+0x44c>
    2cb2:	vmovaps 0x3726(%rip),%ymm7        # 63e0 <_fini+0x46c>
    2cba:	jmp    2cdb <_mlir_ciface_main_graph_model+0x12b>
    2cbc:	nopl   0x0(%rax)
    2cc0:	inc    %rsi
    2cc3:	add    $0x10000,%rdx
    2cca:	add    $0x100,%r8
    2cd1:	cmp    $0xc,%rsi
    2cd5:	je     337a <_mlir_ciface_main_graph_model+0x7ca>
    2cdb:	mov    %rsi,%rcx
    2cde:	shl    $0x10,%rcx
    2ce2:	lea    (%rax,%rcx,1),%r9
    2ce6:	lea    0x10000(%rax,%rcx,1),%rcx
    2cee:	mov    %rsi,%r10
    2cf1:	shl    $0x8,%r10
    2cf5:	lea    0xbf500(%r14,%r10,1),%r11
    2cfd:	cmp    %r11,%r9
    2d00:	setb   %bpl
    2d04:	add    %r14,%r10
    2d07:	cmp    %rcx,%r10
    2d0a:	setb   %r11b
    2d0e:	and    %bpl,%r11b
    2d11:	mov    %r8,%r15
    2d14:	mov    %rdx,%r12
    2d17:	xor    %r13d,%r13d
    2d1a:	jmp    32b0 <_mlir_ciface_main_graph_model+0x700>
    2d1f:	nop
    2d20:	lea    (%r10,%r13,4),%rcx
    2d24:	vxorps %xmm16,%xmm16,%xmm16
    2d2a:	kxnorb %k0,%k0,%k1
    2d2e:	vmovaps 0x32c8(%rip),%ymm17        # 6000 <_fini+0x8c>
    2d38:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k1}
    2d3f:	vxorps %xmm17,%xmm17,%xmm17
    2d45:	kxnorb %k0,%k0,%k1
    2d49:	vmovaps 0x32cd(%rip),%ymm18        # 6020 <_fini+0xac>
    2d53:	vgatherdps (%rcx,%ymm18,1),%ymm17{%k1}
    2d5a:	vxorps %xmm18,%xmm18,%xmm18
    2d60:	kxnorb %k0,%k0,%k1
    2d64:	vmovaps 0x32d2(%rip),%ymm19        # 6040 <_fini+0xcc>
    2d6e:	vgatherdps (%rcx,%ymm19,1),%ymm18{%k1}
    2d75:	vxorps %xmm19,%xmm19,%xmm19
    2d7b:	kxnorb %k0,%k0,%k1
    2d7f:	vmovaps 0x32d7(%rip),%ymm20        # 6060 <_fini+0xec>
    2d89:	vgatherdps (%rcx,%ymm20,1),%ymm19{%k1}
    2d90:	vxorps %xmm20,%xmm20,%xmm20
    2d96:	kxnorb %k0,%k0,%k1
    2d9a:	vmovaps 0x32dc(%rip),%ymm21        # 6080 <_fini+0x10c>
    2da4:	vgatherdps (%rcx,%ymm21,1),%ymm20{%k1}
    2dab:	vxorps %xmm21,%xmm21,%xmm21
    2db1:	kxnorb %k0,%k0,%k1
    2db5:	vmovaps 0x32e1(%rip),%ymm22        # 60a0 <_fini+0x12c>
    2dbf:	vgatherdps (%rcx,%ymm22,1),%ymm21{%k1}
    2dc6:	vxorps %xmm22,%xmm22,%xmm22
    2dcc:	kxnorb %k0,%k0,%k1
    2dd0:	vmovaps 0x32e6(%rip),%ymm23        # 60c0 <_fini+0x14c>
    2dda:	vgatherdps (%rcx,%ymm23,1),%ymm22{%k1}
    2de1:	vxorps %xmm23,%xmm23,%xmm23
    2de7:	kxnorb %k0,%k0,%k1
    2deb:	vmovaps 0x32eb(%rip),%ymm24        # 60e0 <_fini+0x16c>
    2df5:	vgatherdps (%rcx,%ymm24,1),%ymm23{%k1}
    2dfc:	mov    %r13,%rbp
    2dff:	shl    $0xa,%rbp
    2e03:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    2e0a:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    2e11:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    2e18:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    2e1f:	vmovaps %zmm18,%zmm20
    2e25:	vpermt2ps %zmm19,%zmm8,%zmm20
    2e2b:	vmovaps %zmm16,%zmm21
    2e31:	vpermt2ps %zmm17,%zmm9,%zmm21
    2e37:	kmovd  %edi,%k1
    2e3b:	vmovapd %zmm20,%zmm21{%k1}
    2e41:	vmovaps %zmm18,%zmm20
    2e47:	vpermt2ps %zmm19,%zmm10,%zmm20
    2e4d:	vmovaps %zmm18,%zmm22
    2e53:	vpermt2ps %zmm19,%zmm12,%zmm22
    2e59:	vmovaps %zmm16,%zmm23
    2e5f:	vpermt2ps %zmm17,%zmm13,%zmm23
    2e65:	vmovapd %zmm22,%zmm23{%k1}
    2e6b:	vmovaps %zmm16,%zmm22
    2e71:	vpermt2ps %zmm17,%zmm11,%zmm22
    2e77:	vpermt2ps %zmm19,%zmm14,%zmm18
    2e7d:	vpermt2ps %zmm17,%zmm15,%zmm16
    2e83:	vmovapd %zmm18,%zmm16{%k1}
    2e89:	vmovupd %zmm16,0xc0(%r9,%rbp,1)
    2e91:	vmovupd %zmm23,0x80(%r9,%rbp,1)
    2e99:	vmovapd %zmm20,%zmm22{%k1}
    2e9f:	vmovupd %zmm22,0x40(%r9,%rbp,1)
    2ea7:	vmovupd %zmm21,(%r9,%rbp,1)
    2eae:	vxorpd %xmm16,%xmm16,%xmm16
    2eb4:	kxnorb %k0,%k0,%k2
    2eb8:	vmovaps 0x323e(%rip),%ymm17        # 6100 <_fini+0x18c>
    2ec2:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k2}
    2ec9:	vxorps %xmm17,%xmm17,%xmm17
    2ecf:	kxnorb %k0,%k0,%k2
    2ed3:	vmovaps 0x3243(%rip),%ymm18        # 6120 <_fini+0x1ac>
    2edd:	vgatherdps (%rcx,%ymm18,1),%ymm17{%k2}
    2ee4:	vxorps %xmm18,%xmm18,%xmm18
    2eea:	kxnorb %k0,%k0,%k2
    2eee:	vmovaps 0x3248(%rip),%ymm19        # 6140 <_fini+0x1cc>
    2ef8:	vgatherdps (%rcx,%ymm19,1),%ymm18{%k2}
    2eff:	vxorps %xmm19,%xmm19,%xmm19
    2f05:	kxnorb %k0,%k0,%k2
    2f09:	vmovaps 0x324d(%rip),%ymm20        # 6160 <_fini+0x1ec>
    2f13:	vgatherdps (%rcx,%ymm20,1),%ymm19{%k2}
    2f1a:	vxorps %xmm20,%xmm20,%xmm20
    2f20:	kxnorb %k0,%k0,%k2
    2f24:	vmovaps 0x3252(%rip),%ymm21        # 6180 <_fini+0x20c>
    2f2e:	vgatherdps (%rcx,%ymm21,1),%ymm20{%k2}
    2f35:	vxorps %xmm21,%xmm21,%xmm21
    2f3b:	kxnorb %k0,%k0,%k2
    2f3f:	vmovaps 0x3257(%rip),%ymm22        # 61a0 <_fini+0x22c>
    2f49:	vgatherdps (%rcx,%ymm22,1),%ymm21{%k2}
    2f50:	vxorps %xmm22,%xmm22,%xmm22
    2f56:	kxnorb %k0,%k0,%k2
    2f5a:	vmovaps 0x325c(%rip),%ymm23        # 61c0 <_fini+0x24c>
    2f64:	vgatherdps (%rcx,%ymm23,1),%ymm22{%k2}
    2f6b:	vxorps %xmm23,%xmm23,%xmm23
    2f71:	kxnorb %k0,%k0,%k2
    2f75:	vmovaps 0x3261(%rip),%ymm24        # 61e0 <_fini+0x26c>
    2f7f:	vgatherdps (%rcx,%ymm24,1),%ymm23{%k2}
    2f86:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    2f8d:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    2f94:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    2f9b:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    2fa2:	vmovaps %zmm18,%zmm20
    2fa8:	vpermt2ps %zmm19,%zmm8,%zmm20
    2fae:	vmovaps %zmm16,%zmm21
    2fb4:	vpermt2ps %zmm17,%zmm9,%zmm21
    2fba:	vmovapd %zmm20,%zmm21{%k1}
    2fc0:	vmovaps %zmm18,%zmm20
    2fc6:	vpermt2ps %zmm19,%zmm10,%zmm20
    2fcc:	vmovaps %zmm18,%zmm22
    2fd2:	vpermt2ps %zmm19,%zmm12,%zmm22
    2fd8:	vmovaps %zmm16,%zmm23
    2fde:	vpermt2ps %zmm17,%zmm13,%zmm23
    2fe4:	vmovapd %zmm22,%zmm23{%k1}
    2fea:	vmovaps %zmm16,%zmm22
    2ff0:	vpermt2ps %zmm17,%zmm11,%zmm22
    2ff6:	vpermt2ps %zmm19,%zmm14,%zmm18
    2ffc:	vpermt2ps %zmm17,%zmm15,%zmm16
    3002:	vmovapd %zmm18,%zmm16{%k1}
    3008:	vmovupd %zmm16,0x1c0(%r9,%rbp,1)
    3010:	vmovupd %zmm23,0x180(%r9,%rbp,1)
    3018:	vmovapd %zmm20,%zmm22{%k1}
    301e:	vmovupd %zmm22,0x140(%r9,%rbp,1)
    3026:	vmovupd %zmm21,0x100(%r9,%rbp,1)
    302e:	vxorpd %xmm16,%xmm16,%xmm16
    3034:	kxnorb %k0,%k0,%k2
    3038:	vmovaps 0x31be(%rip),%ymm17        # 6200 <_fini+0x28c>
    3042:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k2}
    3049:	vxorps %xmm17,%xmm17,%xmm17
    304f:	kxnorb %k0,%k0,%k2
    3053:	vgatherdps (%rcx,%ymm25,1),%ymm17{%k2}
    305a:	vxorps %xmm18,%xmm18,%xmm18
    3060:	kxnorb %k0,%k0,%k2
    3064:	vgatherdps (%rcx,%ymm26,1),%ymm18{%k2}
    306b:	vxorps %xmm19,%xmm19,%xmm19
    3071:	kxnorb %k0,%k0,%k2
    3075:	vgatherdps (%rcx,%ymm27,1),%ymm19{%k2}
    307c:	vxorps %xmm20,%xmm20,%xmm20
    3082:	kxnorb %k0,%k0,%k2
    3086:	vgatherdps (%rcx,%ymm28,1),%ymm20{%k2}
    308d:	vxorpd %xmm21,%xmm21,%xmm21
    3093:	kxnorb %k0,%k0,%k2
    3097:	vgatherdps (%rcx,%ymm29,1),%ymm21{%k2}
    309e:	vxorpd %xmm22,%xmm22,%xmm22
    30a4:	kxnorb %k0,%k0,%k2
    30a8:	vgatherdps (%rcx,%ymm30,1),%ymm22{%k2}
    30af:	vxorpd %xmm23,%xmm23,%xmm23
    30b5:	kxnorb %k0,%k0,%k2
    30b9:	vgatherdps (%rcx,%ymm31,1),%ymm23{%k2}
    30c0:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    30c7:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    30ce:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    30d5:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    30dc:	vmovaps %zmm18,%zmm20
    30e2:	vpermt2ps %zmm19,%zmm8,%zmm20
    30e8:	vmovaps %zmm16,%zmm21
    30ee:	vpermt2ps %zmm17,%zmm9,%zmm21
    30f4:	vmovapd %zmm20,%zmm21{%k1}
    30fa:	vmovaps %zmm18,%zmm20
    3100:	vpermt2ps %zmm19,%zmm10,%zmm20
    3106:	vmovaps %zmm18,%zmm22
    310c:	vpermt2ps %zmm19,%zmm12,%zmm22
    3112:	vmovaps %zmm16,%zmm23
    3118:	vpermt2ps %zmm17,%zmm13,%zmm23
    311e:	vmovapd %zmm22,%zmm23{%k1}
    3124:	vmovaps %zmm16,%zmm22
    312a:	vpermt2ps %zmm17,%zmm11,%zmm22
    3130:	vpermt2ps %zmm19,%zmm14,%zmm18
    3136:	vpermt2ps %zmm17,%zmm15,%zmm16
    313c:	vmovapd %zmm18,%zmm16{%k1}
    3142:	vmovupd %zmm16,0x2c0(%r9,%rbp,1)
    314a:	vmovupd %zmm23,0x280(%r9,%rbp,1)
    3152:	vmovapd %zmm20,%zmm22{%k1}
    3158:	vmovupd %zmm22,0x240(%r9,%rbp,1)
    3160:	vmovupd %zmm21,0x200(%r9,%rbp,1)
    3168:	vxorpd %xmm16,%xmm16,%xmm16
    316e:	kxnorb %k0,%k0,%k2
    3172:	vgatherdps (%rcx,%ymm0,1),%ymm16{%k2}
    3179:	vxorps %xmm17,%xmm17,%xmm17
    317f:	kxnorb %k0,%k0,%k2
    3183:	vgatherdps (%rcx,%ymm1,1),%ymm17{%k2}
    318a:	vxorps %xmm18,%xmm18,%xmm18
    3190:	kxnorb %k0,%k0,%k2
    3194:	vgatherdps (%rcx,%ymm2,1),%ymm18{%k2}
    319b:	vxorps %xmm19,%xmm19,%xmm19
    31a1:	kxnorb %k0,%k0,%k2
    31a5:	vgatherdps (%rcx,%ymm3,1),%ymm19{%k2}
    31ac:	vxorps %xmm20,%xmm20,%xmm20
    31b2:	kxnorb %k0,%k0,%k2
    31b6:	vgatherdps (%rcx,%ymm4,1),%ymm20{%k2}
    31bd:	vxorpd %xmm21,%xmm21,%xmm21
    31c3:	kxnorb %k0,%k0,%k2
    31c7:	vgatherdps (%rcx,%ymm5,1),%ymm21{%k2}
    31ce:	vxorpd %xmm22,%xmm22,%xmm22
    31d4:	kxnorb %k0,%k0,%k2
    31d8:	vgatherdps (%rcx,%ymm6,1),%ymm22{%k2}
    31df:	vxorpd %xmm23,%xmm23,%xmm23
    31e5:	kxnorb %k0,%k0,%k2
    31e9:	vgatherdps (%rcx,%ymm7,1),%ymm23{%k2}
    31f0:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    31f7:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    31fe:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    3205:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    320c:	vmovaps %zmm18,%zmm20
    3212:	vpermt2ps %zmm19,%zmm8,%zmm20
    3218:	vmovaps %zmm16,%zmm21
    321e:	vpermt2ps %zmm17,%zmm9,%zmm21
    3224:	vmovapd %zmm20,%zmm21{%k1}
    322a:	vmovaps %zmm18,%zmm20
    3230:	vpermt2ps %zmm19,%zmm10,%zmm20
    3236:	vmovaps %zmm16,%zmm22
    323c:	vpermt2ps %zmm17,%zmm11,%zmm22
    3242:	vmovapd %zmm20,%zmm22{%k1}
    3248:	vmovaps %zmm18,%zmm20
    324e:	vpermt2ps %zmm19,%zmm12,%zmm20
    3254:	vmovaps %zmm16,%zmm23
    325a:	vpermt2ps %zmm17,%zmm13,%zmm23
    3260:	vmovapd %zmm20,%zmm23{%k1}
    3266:	vpermt2ps %zmm19,%zmm14,%zmm18
    326c:	vpermt2ps %zmm17,%zmm15,%zmm16
    3272:	vmovapd %zmm18,%zmm16{%k1}
    3278:	vmovupd %zmm16,0x3c0(%r9,%rbp,1)
    3280:	vmovupd %zmm23,0x380(%r9,%rbp,1)
    3288:	vmovupd %zmm22,0x340(%r9,%rbp,1)
    3290:	vmovupd %zmm21,0x300(%r9,%rbp,1)
    3298:	inc    %r13
    329b:	add    $0x400,%r12
    32a2:	add    $0x4,%r15
    32a6:	cmp    $0x40,%r13
    32aa:	je     2cc0 <_mlir_ciface_main_graph_model+0x110>
    32b0:	test   %r11b,%r11b
    32b3:	je     2d20 <_mlir_ciface_main_graph_model+0x170>
    32b9:	mov    $0xfffffffffffffff8,%rcx
    32c0:	mov    %r15,%rbp
    32c3:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    32d0:	vmovss 0x0(%rbp),%xmm16
    32d7:	vmovss %xmm16,0x4(%r12,%rcx,4)
    32df:	vmovss 0xc00(%rbp),%xmm16
    32e9:	vmovss %xmm16,0x8(%r12,%rcx,4)
    32f1:	vmovss 0x1800(%rbp),%xmm16
    32fb:	vmovss %xmm16,0xc(%r12,%rcx,4)
    3303:	vmovss 0x2400(%rbp),%xmm16
    330d:	vmovss %xmm16,0x10(%r12,%rcx,4)
    3315:	vmovss 0x3000(%rbp),%xmm16
    331f:	vmovss %xmm16,0x14(%r12,%rcx,4)
    3327:	vmovss 0x3c00(%rbp),%xmm16
    3331:	vmovss %xmm16,0x18(%r12,%rcx,4)
    3339:	vmovss 0x4800(%rbp),%xmm16
    3343:	vmovss %xmm16,0x1c(%r12,%rcx,4)
    334b:	vmovss 0x5400(%rbp),%xmm16
    3355:	vmovss %xmm16,0x20(%r12,%rcx,4)
    335d:	add    $0x8,%rcx
    3361:	add    $0x6000,%rbp
    3368:	cmp    $0xf8,%rcx
    336f:	jb     32d0 <_mlir_ciface_main_graph_model+0x720>
    3375:	jmp    3298 <_mlir_ciface_main_graph_model+0x6e8>
    337a:	mov    (%rsp),%rcx
    337e:	mov    %rcx,(%rbx)
    3381:	mov    %rax,0x8(%rbx)
    3385:	vmovaps 0x30b3(%rip),%ymm0        # 6440 <_fini+0x4cc>
    338d:	vmovups %ymm0,0x10(%rbx)
    3392:	vmovaps 0x30c6(%rip),%ymm0        # 6460 <_fini+0x4ec>
    339a:	vmovups %ymm0,0x30(%rbx)
    339f:	movq   $0x1,0x50(%rbx)
    33a7:	add    $0x8,%rsp
    33ab:	pop    %rbx
    33ac:	pop    %r12
    33ae:	pop    %r13
    33b0:	pop    %r14
    33b2:	pop    %r15
    33b4:	pop    %rbp
    33b5:	vzeroupper
    33b8:	ret
    33b9:	nopl   0x0(%rax)
## _mlir_ciface_main_graph_model@plt
    2150:	jmp    *0x6f3a(%rip)        # 9090 <_mlir_ciface_main_graph_model@@Base+0x64e0>
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
    23b1:	mov    $0xc0010,%edi
    23b6:	call   21d0 <malloc@plt>
    23bb:	mov    %rax,(%rsp)
    23bf:	add    $0xf,%rax
    23c3:	and    $0xfffffffffffffff0,%rax
    23c7:	lea    0x1c(%rax),%rdx
    23cb:	xor    %esi,%esi
    23cd:	vmovaps 0x40a9(%rip),%zmm8        # 6480 <_fini+0x50c>
    23d7:	vmovaps 0x40df(%rip),%zmm9        # 64c0 <_fini+0x54c>
    23e1:	mov    $0xcc,%dil
    23e4:	vmovaps 0x4112(%rip),%zmm10        # 6500 <_fini+0x58c>
    23ee:	vmovaps 0x4148(%rip),%zmm11        # 6540 <_fini+0x5cc>
    23f8:	vmovaps 0x417e(%rip),%zmm12        # 6580 <_fini+0x60c>
    2402:	vmovaps 0x41b4(%rip),%zmm13        # 65c0 <_fini+0x64c>
    240c:	vmovaps 0x41ea(%rip),%zmm14        # 6600 <_fini+0x68c>
    2416:	vmovaps 0x4220(%rip),%zmm15        # 6640 <_fini+0x6cc>
    2420:	vmovaps 0x3df6(%rip),%ymm25        # 6220 <_fini+0x2ac>
    242a:	vmovaps 0x3e0c(%rip),%ymm26        # 6240 <_fini+0x2cc>
    2434:	vmovaps 0x3e22(%rip),%ymm27        # 6260 <_fini+0x2ec>
    243e:	vmovaps 0x3e38(%rip),%ymm28        # 6280 <_fini+0x30c>
    2448:	vmovaps 0x3e4e(%rip),%ymm29        # 62a0 <_fini+0x32c>
    2452:	vmovaps 0x3e64(%rip),%ymm30        # 62c0 <_fini+0x34c>
    245c:	vmovaps 0x3e7a(%rip),%ymm31        # 62e0 <_fini+0x36c>
    2466:	mov    %r14,%r8
    2469:	vmovaps 0x3e8f(%rip),%ymm0        # 6300 <_fini+0x38c>
    2471:	vmovaps 0x3ea7(%rip),%ymm1        # 6320 <_fini+0x3ac>
    2479:	vmovaps 0x3ebf(%rip),%ymm2        # 6340 <_fini+0x3cc>
    2481:	vmovaps 0x3ed7(%rip),%ymm3        # 6360 <_fini+0x3ec>
    2489:	vmovaps 0x3eef(%rip),%ymm4        # 6380 <_fini+0x40c>
    2491:	vmovaps 0x3f07(%rip),%ymm5        # 63a0 <_fini+0x42c>
    2499:	vmovaps 0x3f1f(%rip),%ymm6        # 63c0 <_fini+0x44c>
    24a1:	vmovaps 0x3f37(%rip),%ymm7        # 63e0 <_fini+0x46c>
    24a9:	jmp    24cb <main_graph_model+0x12b>
    24ab:	nopl   0x0(%rax,%rax,1)
    24b0:	inc    %rsi
    24b3:	add    $0x100,%r8
    24ba:	add    $0x10000,%rdx
    24c1:	cmp    $0xc,%rsi
    24c5:	je     2b6a <main_graph_model+0x7ca>
    24cb:	mov    %rsi,%rcx
    24ce:	shl    $0x10,%rcx
    24d2:	lea    (%rax,%rcx,1),%r9
    24d6:	lea    0x10000(%rax,%rcx,1),%rcx
    24de:	mov    %rsi,%r10
    24e1:	shl    $0x8,%r10
    24e5:	lea    0xbf500(%r14,%r10,1),%r11
    24ed:	cmp    %r11,%r9
    24f0:	setb   %bpl
    24f4:	add    %r14,%r10
    24f7:	cmp    %rcx,%r10
    24fa:	setb   %r11b
    24fe:	and    %bpl,%r11b
    2501:	mov    %rdx,%r15
    2504:	mov    %r8,%r12
    2507:	xor    %r13d,%r13d
    250a:	jmp    2aa0 <main_graph_model+0x700>
    250f:	nop
    2510:	lea    (%r10,%r13,4),%rcx
    2514:	kxnorb %k0,%k0,%k1
    2518:	vxorps %xmm16,%xmm16,%xmm16
    251e:	vmovaps 0x3ad8(%rip),%ymm17        # 6000 <_fini+0x8c>
    2528:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k1}
    252f:	kxnorb %k0,%k0,%k1
    2533:	vxorps %xmm17,%xmm17,%xmm17
    2539:	vmovaps 0x3add(%rip),%ymm18        # 6020 <_fini+0xac>
    2543:	vgatherdps (%rcx,%ymm18,1),%ymm17{%k1}
    254a:	kxnorb %k0,%k0,%k1
    254e:	vxorps %xmm18,%xmm18,%xmm18
    2554:	vmovaps 0x3ae2(%rip),%ymm19        # 6040 <_fini+0xcc>
    255e:	vgatherdps (%rcx,%ymm19,1),%ymm18{%k1}
    2565:	kxnorb %k0,%k0,%k1
    2569:	vxorps %xmm19,%xmm19,%xmm19
    256f:	vmovaps 0x3ae7(%rip),%ymm20        # 6060 <_fini+0xec>
    2579:	vgatherdps (%rcx,%ymm20,1),%ymm19{%k1}
    2580:	kxnorb %k0,%k0,%k1
    2584:	vxorps %xmm20,%xmm20,%xmm20
    258a:	vmovaps 0x3aec(%rip),%ymm21        # 6080 <_fini+0x10c>
    2594:	vgatherdps (%rcx,%ymm21,1),%ymm20{%k1}
    259b:	kxnorb %k0,%k0,%k1
    259f:	vxorps %xmm21,%xmm21,%xmm21
    25a5:	vmovaps 0x3af1(%rip),%ymm22        # 60a0 <_fini+0x12c>
    25af:	vgatherdps (%rcx,%ymm22,1),%ymm21{%k1}
    25b6:	kxnorb %k0,%k0,%k1
    25ba:	vxorps %xmm22,%xmm22,%xmm22
    25c0:	vmovaps 0x3af6(%rip),%ymm23        # 60c0 <_fini+0x14c>
    25ca:	vgatherdps (%rcx,%ymm23,1),%ymm22{%k1}
    25d1:	kxnorb %k0,%k0,%k1
    25d5:	vxorps %xmm23,%xmm23,%xmm23
    25db:	vmovaps 0x3afb(%rip),%ymm24        # 60e0 <_fini+0x16c>
    25e5:	vgatherdps (%rcx,%ymm24,1),%ymm23{%k1}
    25ec:	mov    %r13,%rbp
    25ef:	shl    $0xa,%rbp
    25f3:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    25fa:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    2601:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    2608:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    260f:	vmovaps %zmm18,%zmm20
    2615:	vpermt2ps %zmm19,%zmm8,%zmm20
    261b:	vmovaps %zmm16,%zmm21
    2621:	vpermt2ps %zmm17,%zmm9,%zmm21
    2627:	kmovd  %edi,%k1
    262b:	vmovapd %zmm20,%zmm21{%k1}
    2631:	vmovaps %zmm18,%zmm20
    2637:	vpermt2ps %zmm19,%zmm10,%zmm20
    263d:	vmovaps %zmm18,%zmm22
    2643:	vpermt2ps %zmm19,%zmm12,%zmm22
    2649:	vmovaps %zmm16,%zmm23
    264f:	vpermt2ps %zmm17,%zmm13,%zmm23
    2655:	vmovapd %zmm22,%zmm23{%k1}
    265b:	vmovaps %zmm16,%zmm22
    2661:	vpermt2ps %zmm17,%zmm11,%zmm22
    2667:	vpermt2ps %zmm19,%zmm14,%zmm18
    266d:	vpermt2ps %zmm17,%zmm15,%zmm16
    2673:	vmovapd %zmm18,%zmm16{%k1}
    2679:	vmovupd %zmm16,0xc0(%r9,%rbp,1)
    2681:	vmovupd %zmm23,0x80(%r9,%rbp,1)
    2689:	vmovapd %zmm20,%zmm22{%k1}
    268f:	vmovupd %zmm22,0x40(%r9,%rbp,1)
    2697:	vmovupd %zmm21,(%r9,%rbp,1)
    269e:	kxnorb %k0,%k0,%k2
    26a2:	vxorpd %xmm16,%xmm16,%xmm16
    26a8:	vmovaps 0x3a4e(%rip),%ymm17        # 6100 <_fini+0x18c>
    26b2:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k2}
    26b9:	kxnorb %k0,%k0,%k2
    26bd:	vxorps %xmm17,%xmm17,%xmm17
    26c3:	vmovaps 0x3a53(%rip),%ymm18        # 6120 <_fini+0x1ac>
    26cd:	vgatherdps (%rcx,%ymm18,1),%ymm17{%k2}
    26d4:	kxnorb %k0,%k0,%k2
    26d8:	vxorps %xmm18,%xmm18,%xmm18
    26de:	vmovaps 0x3a58(%rip),%ymm19        # 6140 <_fini+0x1cc>
    26e8:	vgatherdps (%rcx,%ymm19,1),%ymm18{%k2}
    26ef:	kxnorb %k0,%k0,%k2
    26f3:	vxorps %xmm19,%xmm19,%xmm19
    26f9:	vmovaps 0x3a5d(%rip),%ymm20        # 6160 <_fini+0x1ec>
    2703:	vgatherdps (%rcx,%ymm20,1),%ymm19{%k2}
    270a:	kxnorb %k0,%k0,%k2
    270e:	vxorps %xmm20,%xmm20,%xmm20
    2714:	vmovaps 0x3a62(%rip),%ymm21        # 6180 <_fini+0x20c>
    271e:	vgatherdps (%rcx,%ymm21,1),%ymm20{%k2}
    2725:	kxnorb %k0,%k0,%k2
    2729:	vxorps %xmm21,%xmm21,%xmm21
    272f:	vmovaps 0x3a67(%rip),%ymm22        # 61a0 <_fini+0x22c>
    2739:	vgatherdps (%rcx,%ymm22,1),%ymm21{%k2}
    2740:	kxnorb %k0,%k0,%k2
    2744:	vxorps %xmm22,%xmm22,%xmm22
    274a:	vmovaps 0x3a6c(%rip),%ymm23        # 61c0 <_fini+0x24c>
    2754:	vgatherdps (%rcx,%ymm23,1),%ymm22{%k2}
    275b:	kxnorb %k0,%k0,%k2
    275f:	vxorps %xmm23,%xmm23,%xmm23
    2765:	vmovaps 0x3a71(%rip),%ymm24        # 61e0 <_fini+0x26c>
    276f:	vgatherdps (%rcx,%ymm24,1),%ymm23{%k2}
    2776:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    277d:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    2784:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    278b:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    2792:	vmovaps %zmm18,%zmm20
    2798:	vpermt2ps %zmm19,%zmm8,%zmm20
    279e:	vmovaps %zmm16,%zmm21
    27a4:	vpermt2ps %zmm17,%zmm9,%zmm21
    27aa:	vmovapd %zmm20,%zmm21{%k1}
    27b0:	vmovaps %zmm18,%zmm20
    27b6:	vpermt2ps %zmm19,%zmm10,%zmm20
    27bc:	vmovaps %zmm18,%zmm22
    27c2:	vpermt2ps %zmm19,%zmm12,%zmm22
    27c8:	vmovaps %zmm16,%zmm23
    27ce:	vpermt2ps %zmm17,%zmm13,%zmm23
    27d4:	vmovapd %zmm22,%zmm23{%k1}
    27da:	vmovaps %zmm16,%zmm22
    27e0:	vpermt2ps %zmm17,%zmm11,%zmm22
    27e6:	vpermt2ps %zmm19,%zmm14,%zmm18
    27ec:	vpermt2ps %zmm17,%zmm15,%zmm16
    27f2:	vmovapd %zmm18,%zmm16{%k1}
    27f8:	vmovupd %zmm16,0x1c0(%r9,%rbp,1)
    2800:	vmovupd %zmm23,0x180(%r9,%rbp,1)
    2808:	vmovapd %zmm20,%zmm22{%k1}
    280e:	vmovupd %zmm22,0x140(%r9,%rbp,1)
    2816:	vmovupd %zmm21,0x100(%r9,%rbp,1)
    281e:	kxnorb %k0,%k0,%k2
    2822:	vxorpd %xmm16,%xmm16,%xmm16
    2828:	vmovaps 0x39ce(%rip),%ymm17        # 6200 <_fini+0x28c>
    2832:	vgatherdps (%rcx,%ymm17,1),%ymm16{%k2}
    2839:	kxnorb %k0,%k0,%k2
    283d:	vxorps %xmm17,%xmm17,%xmm17
    2843:	vgatherdps (%rcx,%ymm25,1),%ymm17{%k2}
    284a:	kxnorb %k0,%k0,%k2
    284e:	vxorps %xmm18,%xmm18,%xmm18
    2854:	vgatherdps (%rcx,%ymm26,1),%ymm18{%k2}
    285b:	kxnorb %k0,%k0,%k2
    285f:	vxorps %xmm19,%xmm19,%xmm19
    2865:	vgatherdps (%rcx,%ymm27,1),%ymm19{%k2}
    286c:	kxnorb %k0,%k0,%k2
    2870:	vxorps %xmm20,%xmm20,%xmm20
    2876:	vgatherdps (%rcx,%ymm28,1),%ymm20{%k2}
    287d:	kxnorb %k0,%k0,%k2
    2881:	vxorpd %xmm21,%xmm21,%xmm21
    2887:	vgatherdps (%rcx,%ymm29,1),%ymm21{%k2}
    288e:	kxnorb %k0,%k0,%k2
    2892:	vxorpd %xmm22,%xmm22,%xmm22
    2898:	vgatherdps (%rcx,%ymm30,1),%ymm22{%k2}
    289f:	kxnorb %k0,%k0,%k2
    28a3:	vxorpd %xmm23,%xmm23,%xmm23
    28a9:	vgatherdps (%rcx,%ymm31,1),%ymm23{%k2}
    28b0:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    28b7:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    28be:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    28c5:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    28cc:	vmovaps %zmm18,%zmm20
    28d2:	vpermt2ps %zmm19,%zmm8,%zmm20
    28d8:	vmovaps %zmm16,%zmm21
    28de:	vpermt2ps %zmm17,%zmm9,%zmm21
    28e4:	vmovapd %zmm20,%zmm21{%k1}
    28ea:	vmovaps %zmm18,%zmm20
    28f0:	vpermt2ps %zmm19,%zmm10,%zmm20
    28f6:	vmovaps %zmm18,%zmm22
    28fc:	vpermt2ps %zmm19,%zmm12,%zmm22
    2902:	vmovaps %zmm16,%zmm23
    2908:	vpermt2ps %zmm17,%zmm13,%zmm23
    290e:	vmovapd %zmm22,%zmm23{%k1}
    2914:	vmovaps %zmm16,%zmm22
    291a:	vpermt2ps %zmm17,%zmm11,%zmm22
    2920:	vpermt2ps %zmm19,%zmm14,%zmm18
    2926:	vpermt2ps %zmm17,%zmm15,%zmm16
    292c:	vmovapd %zmm18,%zmm16{%k1}
    2932:	vmovupd %zmm16,0x2c0(%r9,%rbp,1)
    293a:	vmovupd %zmm23,0x280(%r9,%rbp,1)
    2942:	vmovapd %zmm20,%zmm22{%k1}
    2948:	vmovupd %zmm22,0x240(%r9,%rbp,1)
    2950:	vmovupd %zmm21,0x200(%r9,%rbp,1)
    2958:	kxnorb %k0,%k0,%k2
    295c:	vxorpd %xmm16,%xmm16,%xmm16
    2962:	vgatherdps (%rcx,%ymm0,1),%ymm16{%k2}
    2969:	kxnorb %k0,%k0,%k2
    296d:	vxorps %xmm17,%xmm17,%xmm17
    2973:	vgatherdps (%rcx,%ymm1,1),%ymm17{%k2}
    297a:	kxnorb %k0,%k0,%k2
    297e:	vxorps %xmm18,%xmm18,%xmm18
    2984:	vgatherdps (%rcx,%ymm2,1),%ymm18{%k2}
    298b:	kxnorb %k0,%k0,%k2
    298f:	vxorps %xmm19,%xmm19,%xmm19
    2995:	vgatherdps (%rcx,%ymm3,1),%ymm19{%k2}
    299c:	kxnorb %k0,%k0,%k2
    29a0:	vxorps %xmm20,%xmm20,%xmm20
    29a6:	vgatherdps (%rcx,%ymm4,1),%ymm20{%k2}
    29ad:	kxnorb %k0,%k0,%k2
    29b1:	vxorpd %xmm21,%xmm21,%xmm21
    29b7:	vgatherdps (%rcx,%ymm5,1),%ymm21{%k2}
    29be:	kxnorb %k0,%k0,%k2
    29c2:	vxorpd %xmm22,%xmm22,%xmm22
    29c8:	vgatherdps (%rcx,%ymm6,1),%ymm22{%k2}
    29cf:	kxnorb %k0,%k0,%k2
    29d3:	vxorpd %xmm23,%xmm23,%xmm23
    29d9:	vgatherdps (%rcx,%ymm7,1),%ymm23{%k2}
    29e0:	vinsertf64x4 $0x1,%ymm17,%zmm16,%zmm16
    29e7:	vinsertf64x4 $0x1,%ymm19,%zmm18,%zmm17
    29ee:	vinsertf64x4 $0x1,%ymm21,%zmm20,%zmm18
    29f5:	vinsertf64x4 $0x1,%ymm23,%zmm22,%zmm19
    29fc:	vmovaps %zmm18,%zmm20
    2a02:	vpermt2ps %zmm19,%zmm8,%zmm20
    2a08:	vmovaps %zmm16,%zmm21
    2a0e:	vpermt2ps %zmm17,%zmm9,%zmm21
    2a14:	vmovapd %zmm20,%zmm21{%k1}
    2a1a:	vmovaps %zmm18,%zmm20
    2a20:	vpermt2ps %zmm19,%zmm10,%zmm20
    2a26:	vmovaps %zmm16,%zmm22
    2a2c:	vpermt2ps %zmm17,%zmm11,%zmm22
    2a32:	vmovapd %zmm20,%zmm22{%k1}
    2a38:	vmovaps %zmm18,%zmm20
    2a3e:	vpermt2ps %zmm19,%zmm12,%zmm20
    2a44:	vmovaps %zmm16,%zmm23
    2a4a:	vpermt2ps %zmm17,%zmm13,%zmm23
    2a50:	vmovapd %zmm20,%zmm23{%k1}
    2a56:	vpermt2ps %zmm19,%zmm14,%zmm18
    2a5c:	vpermt2ps %zmm17,%zmm15,%zmm16
    2a62:	vmovapd %zmm18,%zmm16{%k1}
    2a68:	vmovupd %zmm16,0x3c0(%r9,%rbp,1)
    2a70:	vmovupd %zmm23,0x380(%r9,%rbp,1)
    2a78:	vmovupd %zmm22,0x340(%r9,%rbp,1)
    2a80:	vmovupd %zmm21,0x300(%r9,%rbp,1)
    2a88:	inc    %r13
    2a8b:	add    $0x4,%r12
    2a8f:	add    $0x400,%r15
    2a96:	cmp    $0x40,%r13
    2a9a:	je     24b0 <main_graph_model+0x110>
    2aa0:	test   %r11b,%r11b
    2aa3:	je     2510 <main_graph_model+0x170>
    2aa9:	mov    $0xfffffffffffffff8,%rcx
    2ab0:	mov    %r12,%rbp
    2ab3:	data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
    2ac0:	vmovss 0x0(%rbp),%xmm16
    2ac7:	vmovss %xmm16,0x4(%r15,%rcx,4)
    2acf:	vmovss 0xc00(%rbp),%xmm16
    2ad9:	vmovss %xmm16,0x8(%r15,%rcx,4)
    2ae1:	vmovss 0x1800(%rbp),%xmm16
    2aeb:	vmovss %xmm16,0xc(%r15,%rcx,4)
    2af3:	vmovss 0x2400(%rbp),%xmm16
    2afd:	vmovss %xmm16,0x10(%r15,%rcx,4)
    2b05:	vmovss 0x3000(%rbp),%xmm16
    2b0f:	vmovss %xmm16,0x14(%r15,%rcx,4)
    2b17:	vmovss 0x3c00(%rbp),%xmm16
    2b21:	vmovss %xmm16,0x18(%r15,%rcx,4)
    2b29:	vmovss 0x4800(%rbp),%xmm16
    2b33:	vmovss %xmm16,0x1c(%r15,%rcx,4)
    2b3b:	vmovss 0x5400(%rbp),%xmm16
    2b45:	vmovss %xmm16,0x20(%r15,%rcx,4)
    2b4d:	add    $0x8,%rcx
    2b51:	add    $0x6000,%rbp
    2b58:	cmp    $0xf8,%rcx
    2b5f:	jb     2ac0 <main_graph_model+0x720>
    2b65:	jmp    2a88 <main_graph_model+0x6e8>
    2b6a:	vmovaps 0x388e(%rip),%ymm0        # 6400 <_fini+0x48c>
    2b72:	vmovups %ymm0,0x38(%rbx)
    2b77:	vmovaps 0x38a1(%rip),%ymm0        # 6420 <_fini+0x4ac>
    2b7f:	vmovups %ymm0,0x18(%rbx)
    2b84:	mov    %rax,0x8(%rbx)
    2b88:	mov    (%rsp),%rax
    2b8c:	mov    %rax,(%rbx)
    2b8f:	movq   $0x0,0x10(%rbx)
    2b97:	mov    %rbx,%rax
    2b9a:	add    $0x8,%rsp
    2b9e:	pop    %rbx
    2b9f:	pop    %r12
    2ba1:	pop    %r13
    2ba3:	pop    %r14
    2ba5:	pop    %r15
    2ba7:	pop    %rbp
    2ba8:	vzeroupper
    2bab:	ret
    2bac:	nopl   0x0(%rax)
## run_main_graph
    3630:	jmp    2070 <run_main_graph_model@plt>
    3635:	data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model
    33c0:	push   %rbp
    33c1:	mov    %rsp,%rbp
    33c4:	push   %r15
    33c6:	push   %r14
    33c8:	push   %r13
    33ca:	push   %r12
    33cc:	push   %rbx
    33cd:	sub    $0x48,%rsp
    33d1:	mov    %rdi,%rbx
    33d4:	call   22a0 <omTensorListGetSize@plt>
    33d9:	cmp    $0x1,%rax
    33dd:	jne    35b1 <run_main_graph_model+0x1f1>
    33e3:	mov    %rbx,%rdi
    33e6:	call   2140 <omTensorListGetOmtArray@plt>
    33eb:	mov    (%rax),%r14
    33ee:	mov    %r14,%rdi
    33f1:	call   22b0 <omTensorGetDataType@plt>
    33f6:	cmp    $0x1,%rax
    33fa:	jne    35c6 <run_main_graph_model+0x206>
    3400:	mov    %r14,%rdi
    3403:	call   20f0 <omTensorGetRank@plt>
    3408:	cmp    $0x4,%rax
    340c:	jne    35f3 <run_main_graph_model+0x233>
    3412:	mov    %r14,%rdi
    3415:	call   20e0 <omTensorGetShape@plt>
    341a:	mov    (%rax),%rsi
    341d:	cmp    $0x1,%rsi
    3421:	jne    35fc <run_main_graph_model+0x23c>
    3427:	mov    0x8(%rax),%rsi
    342b:	cmp    $0x100,%rsi
    3432:	jne    3605 <run_main_graph_model+0x245>
    3438:	mov    0x10(%rax),%rsi
    343c:	cmp    $0xc,%rsi
    3440:	jne    360e <run_main_graph_model+0x24e>
    3446:	mov    0x18(%rax),%rsi
    344a:	cmp    $0x40,%rsi
    344e:	jne    3617 <run_main_graph_model+0x257>
    3454:	mov    %rbx,%rdi
    3457:	call   2140 <omTensorListGetOmtArray@plt>
    345c:	mov    %rsp,%rbx
    345f:	lea    -0x60(%rbx),%rcx
    3463:	mov    %rcx,%rsp
    3466:	mov    (%rax),%r14
    3469:	mov    %rsp,%r15
    346c:	lea    -0x60(%r15),%rax
    3470:	mov    %rax,-0x30(%rbp)
    3474:	mov    %rax,%rsp
    3477:	mov    %r14,%rdi
    347a:	call   20a0 <omTensorGetDataPtr@plt>
    347f:	mov    %rax,%r12
    3482:	mov    %r14,%rdi
    3485:	call   20e0 <omTensorGetShape@plt>
    348a:	mov    %rax,%r13
    348d:	mov    %r14,%rdi
    3490:	call   2120 <omTensorGetStrides@plt>
    3495:	mov    %r12,-0x60(%r15)
    3499:	mov    %r12,-0x58(%r15)
    349d:	movq   $0x0,-0x50(%r15)
    34a5:	vmovups 0x0(%r13),%ymm0
    34ab:	vmovups %ymm0,-0x48(%r15)
    34b1:	vmovups (%rax),%ymm0
    34b5:	vmovups %ymm0,-0x28(%r15)
    34bb:	lea    -0x60(%rbx),%rdi
    34bf:	mov    -0x30(%rbp),%rsi
    34c3:	vzeroupper
    34c6:	call   2150 <_mlir_ciface_main_graph_model@plt>
    34cb:	mov    -0x60(%rbx),%r15
    34cf:	mov    -0x58(%rbx),%r12
    34d3:	mov    -0x48(%rbx),%rax
    34d7:	mov    %rax,-0x58(%rbp)
    34db:	mov    -0x40(%rbx),%rax
    34df:	mov    %rax,-0x60(%rbp)
    34e3:	mov    -0x38(%rbx),%rax
    34e7:	mov    %rax,-0x68(%rbp)
    34eb:	mov    -0x30(%rbx),%rax
    34ef:	mov    %rax,-0x30(%rbp)
    34f3:	mov    -0x28(%rbx),%rax
    34f7:	mov    %rax,-0x38(%rbp)
    34fb:	mov    -0x20(%rbx),%rax
    34ff:	mov    %rax,-0x40(%rbp)
    3503:	mov    -0x18(%rbx),%rax
    3507:	mov    %rax,-0x48(%rbp)
    350b:	mov    -0x10(%rbx),%rax
    350f:	mov    %rax,-0x50(%rbp)
    3513:	mov    %rsp,%r13
    3516:	lea    -0x10(%r13),%rbx
    351a:	mov    %rbx,%rsp
    351d:	mov    $0x4,%edi
    3522:	call   2280 <omTensorCreateUntyped@plt>
    3527:	mov    %rax,%r14
    352a:	mov    $0x1,%esi
    352f:	mov    %rax,%rdi
    3532:	mov    %r15,%rdx
    3535:	mov    %r12,%rcx
    3538:	call   20d0 <omTensorSetDataPtr@plt>
    353d:	mov    $0x1,%esi
    3542:	mov    %r14,%rdi
    3545:	call   2290 <omTensorSetDataType@plt>
    354a:	mov    %r14,%rdi
    354d:	call   20e0 <omTensorGetShape@plt>
    3552:	mov    %rax,%r15
    3555:	mov    %r14,%rdi
    3558:	call   2120 <omTensorGetStrides@plt>
    355d:	mov    -0x58(%rbp),%rcx
    3561:	mov    %rcx,(%r15)
    3564:	mov    -0x38(%rbp),%rcx
    3568:	mov    %rcx,(%rax)
    356b:	mov    -0x60(%rbp),%rcx
    356f:	mov    %rcx,0x8(%r15)
    3573:	mov    -0x40(%rbp),%rcx
    3577:	mov    %rcx,0x8(%rax)
    357b:	mov    -0x68(%rbp),%rcx
    357f:	mov    %rcx,0x10(%r15)
    3583:	mov    -0x48(%rbp),%rcx
    3587:	mov    %rcx,0x10(%rax)
    358b:	mov    -0x30(%rbp),%rcx
    358f:	mov    %rcx,0x18(%r15)
    3593:	mov    -0x50(%rbp),%rcx
    3597:	mov    %rcx,0x18(%rax)
    359b:	mov    %r14,-0x10(%r13)
    359f:	mov    $0x1,%esi
    35a4:	mov    %rbx,%rdi
    35a7:	call   21c0 <omTensorListCreate@plt>
    35ac:	mov    %rax,%rbx
    35af:	jmp    35e1 <run_main_graph_model+0x221>
    35b1:	lea    0x3558(%rip),%rdi        # 6b10 <om_Wrong number of input tensors: expect 1, but got %lld^J_model>
    35b8:	xor    %ebx,%ebx
    35ba:	mov    %rax,%rsi
    35bd:	xor    %eax,%eax
    35bf:	call   2040 <printf@plt>
    35c4:	jmp    35d6 <run_main_graph_model+0x216>
    35c6:	lea    0x3513(%rip),%rdi        # 6ae0 <om_Wrong data type for the input 0: expect f32^J_model>
    35cd:	xor    %ebx,%ebx
    35cf:	xor    %eax,%eax
    35d1:	call   2040 <printf@plt>
    35d6:	call   2030 <__errno_location@plt>
    35db:	movl   $0x16,(%rax)
    35e1:	mov    %rbx,%rax
    35e4:	lea    -0x28(%rbp),%rsp
    35e8:	pop    %rbx
    35e9:	pop    %r12
    35eb:	pop    %r13
    35ed:	pop    %r14
    35ef:	pop    %r15
    35f1:	pop    %rbp
    35f2:	ret
    35f3:	lea    0x34a6(%rip),%rdi        # 6aa0 <om_Wrong rank for the input 0: expect 4, but got %lld^J_model>
    35fa:	jmp    35b8 <run_main_graph_model+0x1f8>
    35fc:	lea    0x344d(%rip),%rdi        # 6a50 <om_Wrong size for the dimension 0 of the input 0: expect 1, but got %lld^J_model>
    3603:	jmp    361e <run_main_graph_model+0x25e>
    3605:	lea    0x33f4(%rip),%rdi        # 6a00 <om_Wrong size for the dimension 1 of the input 0: expect 256, but got %lld^J_model>
    360c:	jmp    361e <run_main_graph_model+0x25e>
    360e:	lea    0x339b(%rip),%rdi        # 69b0 <om_Wrong size for the dimension 2 of the input 0: expect 12, but got %lld^J_model>
    3615:	jmp    361e <run_main_graph_model+0x25e>
    3617:	lea    0x3342(%rip),%rdi        # 6960 <om_Wrong size for the dimension 3 of the input 0: expect 64, but got %lld^J_model>
    361e:	xor    %ebx,%ebx
    3620:	jmp    35bd <run_main_graph_model+0x1fd>
    3622:	data16 data16 data16 data16 cs nopw 0x0(%rax,%rax,1)
## run_main_graph_model@plt
    2070:	jmp    *0x6faa(%rip)        # 9020 <run_main_graph_model@@Base+0x5c60>
    2076:	push   $0x4
    207b:	jmp    2020 <_init+0x20>
