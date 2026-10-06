from edit_lib import *
import re,glob
t=load('04_related.md')
pairs=[('후속 ATVA 2004 (block-local atomicity)','후속 ATVA 2004 (block-local atomicity) [R22] [R23]'),
('Netzer & Miller, LOPLAS 1(1):74–88, 1992 (DOI 10.1145/130616.130623)','Netzer & Miller, LOPLAS 1(1):74–88, 1992 (DOI 10.1145/130616.130623) [R27]'),
('| DCatch, ASPLOS 2017 |','| DCatch, ASPLOS 2017 [R28] |'),
('| FCatch, ASPLOS 2018 (','| FCatch, ASPLOS 2018 [R29] ('),
('Flanagan & Qadeer, PLDI 2003 (type system for atomicity); Lu 등, AVIO, ASPLOS 2006','Flanagan & Qadeer, PLDI 2003 (type system for atomicity) [R30]; Lu 등, AVIO, ASPLOS 2006 [R31]'),
('Schwarz 등, POPL 2011 (priority ceiling interrupt program), VMCAI 2014 (정수 변수로 만든 값 의존 동기화); Chopra 등 (RTOS kernel high-level race)',
 'Schwarz 등, POPL 2011 (priority ceiling interrupt program) [R32], VMCAI 2014 (정수 변수로 만든 값 의존 동기화) [R33]; Chopra 등 (RTOS kernel high-level race) [R34]'),
('WebRacer PLDI 2012, InitRacer OOPSLA 2017','WebRacer PLDI 2012 [R35], InitRacer OOPSLA 2017 [R36]'),
('| DroidRacer, CAFA (PLDI 2014), SIERRA (ASPLOS 2018), nAdroid (CGO 2018;','| DroidRacer [R37], CAFA (PLDI 2014) [R38], SIERRA (ASPLOS 2018) [R39], nAdroid (CGO 2018) [R40] ('),
('PADL 2010·2011 논문은','PADL 2010·2011 논문 [R41]은'),
('Becker 등, RTCSA 2016, JSA 2017 (DOI 10.1016/j.sysarc.2017.09.004)','Becker 등, RTCSA 2016, JSA 2017 (DOI 10.1016/j.sysarc.2017.09.004) [R42]'),
('(Edmaier, 2024)','(Edmaier, 2024 [R43])'),
('| Casini 등, ECRTS 2019 (','| Casini 등, ECRTS 2019 [R44] ('),
("IROS'20 Electrum (","IROS'20 Electrum [R46] ("),
("| HAROS ENASE 2022 (HPL) |","| HAROS ENASE 2022 (HPL) [R48] |"),
('| Teper 등, RTSS 2022 (','| Teper 등, RTSS 2022 [R49] ('),
('(Tiotto 등, CGO 2024;','(Tiotto 등, CGO 2024 [R50];'),
('Fehr 등, PLDI 2025 (DOI 10.1145/3729309); Peng 등, POPL 2026 (DOI 10.1145/3776722)','Fehr 등, PLDI 2025 (DOI 10.1145/3729309) [R51]; Peng 등, POPL 2026 (DOI 10.1145/3776722) [R52]'),
('ISSTA 2025 보고(DOI 10.1145/3728923)','ISSTA 2025 보고(DOI 10.1145/3728923) [R53]'),
('RacerF(ECOOP 2025, pthread data race)','RacerF(ECOOP 2025, pthread data race) [R54]'),
('Garcia 등 ICSE 2020은','Garcia 등 ICSE 2020 [R55]은'),
('Chen 등(arXiv 2507.10235)은','Chen 등(arXiv 2507.10235) [R56]은'),
('ROSCallBaX(FSE 2025)는','ROSCallBaX(FSE 2025) [R57]는'),
('Schalk 등 MILCOM 2022, Falco–Thummala "WannaFly" SMC-IT 2023, Jero 등 SpaceSec 2024, Furgala 등 SpaceSec 2026',
 'Schalk 등 MILCOM 2022 [R60], Falco–Thummala "WannaFly" SMC-IT 2023 [R61], Jero 등 SpaceSec 2024 [R62], Furgala 등 SpaceSec 2026 [R63]'),
('Khor–Lutz RE 2023','Khor–Lutz RE 2023 [R66]'),
('| AUTOSAR TIMEX의 age·reaction LatencyTimingConstraint |','| AUTOSAR TIMEX의 age·reaction LatencyTimingConstraint [S114] |'),
('| SVF (6a4bb08f;','| SVF (6a4bb08f [S148];'),('| Joern (98a815cb;','| Joern (98a815cb [S149];'),("| Frama-C 33.0 'Arsenic' (","| Frama-C 33.0 'Arsenic' [S150] ("),
('FSW-08 발표(NTRS 20090004613','FSW-08 발표(NTRS 20090004613 [S113]'),
("HAROS IRC'19·IROS'20·RoSE'21,","HAROS IRC'19[R45]·IROS'20[R46]·RoSE'21[R47],"),
]
for a,b in pairs:
    c=t.count(a)
    if c<1: print('MISSING',a[:70]); continue
    t=t.replace(a,b,1)
save('04_related.md',t)
# ROSpec paper id
t=load('04_related.md')
i=t.find('| ROSpec (')
j=t.find('논문 **[미확인]**',i)
if i>=0 and j>=0 and j-i<400: t=t[:j]+'논문 [R58] **[미확인]**'+t[j+len('논문 **[미확인]**'):]
else: print('rospec not found')
save('04_related.md',t)
# space before IDs
for f in sorted(glob.glob('work/*.md')):
    if f.endswith('11_refs.md'): continue
    s=open(f).read()
    s=re.sub(r'(?<=[0-9A-Za-z가-힣\)\'’`])\[(R\d\d|S\d\d+)\]',r' [\1]',s)
    open(f,'w').write(s)
print('ok')
