# Technique families relevant to spec-free detection of order violations (lead's brief)

Status: written by the lead from memory to give designers the map. EVERY citation below is [U: from memory,
must be verified before it appears in the final document]. The project's own verified summaries of process-level
race work are in R/plan/methods/*.md, R/baselines/*.md and R/plan/process_race_method_matrix.md; read those for
RacePro, SimRacer, DESCRY and related work.

## A. What an order violation is, empirically
- Lu, Park, Seo, Zhou, "Learning from Mistakes: A Comprehensive Study on Real World Concurrency Bug
  Characteristics", ASPLOS 2008. Non-deadlock concurrency bugs are almost all atomicity violations or order
  violations; an order violation is "the desired order between two (groups of) accesses is flipped". Most bugs
  manifest under a specific partial order of a small number of accesses and threads. The "desired order" is the
  programmer's intent: this is the oracle gap.
- Leesatapornwongsa, Lukman, Lu, Gunawi, "TaxDC", ASPLOS 2016: the same classes at the level of messages and
  nodes in distributed systems (order violations between messages, between a message and a local computation).

## B. Oracles that need no written specification
1. Failure sites as the oracle (consequence first, then look for the interleaving):
   - Zhang, Sun, Lu, "ConMem", ASPLOS 2010: only interleavings that lead to a memory error (NULL dereference,
     uninitialized read, dangling pointer) are reported; several patterns are order violations (use before
     initialization, use after free).
   - Zhang, Lim, Olichandran, Scherpelz, Jin, Lu, Reps, "ConSeq", ASPLOS 2011: start from potential failure
     sites (assertions, error-message calls, infinite loops, wrong outputs), slice backward to the shared reads
     that can change the outcome, then look for a racing write and try to trigger it.
   - Liu et al., "DCatch", ASPLOS 2017 (distributed concurrency bugs): happens-before model over messages and
     threads, then static pruning by IMPACT (keep only conflicts whose result can reach a failure instruction:
     exception, abort, fatal log), then triggering. "FCatch" (ASPLOS 2018) and "CloudRaid" (FSE 2018) are relatives.
2. Both orders executed, results compared (differential):
   - Narayanasamy, Wang, Tigani, Edwards, Calder, "Automatically Classifying Benign and Harmful Data Races
     Using Replay Analysis", PLDI 2007: replay each race in both orders; if the resulting states are the same the
     race is potentially benign, if they differ or one replay fails it is potentially harmful.
   - Kasikci, Zamfir, Candea, "Data Races vs. Data Race Bugs: Telling the Difference with Portend", ASPLOS 2012:
     classifies races by consequence over several paths and schedules ("specification violated" = crash,
     deadlock, etc.; "output differs"; "k-witness harmless"; "single ordering" = the other order is not possible).
   - Laadan, Viennot, Tsai, Blinn, Yang, Nieh, "Pervasive Detection of Process Races in Deployed Systems"
     (RacePro), SOSP 2011: races between system calls on shared kernel objects, detected from a recording, then
     validated by replaying the changed order and checking for failures with built-in checkers.
3. Determinism as the specification:
   - Burnim, Sen, "Asserting and Checking Determinism for Multithreaded Programs", FSE 2009: a parallel region
     should produce equivalent results under all schedules; equivalence is given by a "bridge predicate" so that
     benign differences are tolerated. The predicate is the place where a human would otherwise have to help.
4. Learned or mined invariants:
   - Shi, Park, Yin, Lu, Zhou, Chen, Zheng, "Do I Use the Wrong Definition? DefUse", OOPSLA 2010: definition-use
     invariants learned from passing runs (which writes a read may see); a read that sees an unseen definition is
     a suspect. Depends on the training runs.
   - Yang, Evans, Bhardwaj, Bhat, Das, "Perracotta", ICSE 2006; Li, Zhou, "PR-Miner", FSE 2005: mining temporal
     or co-occurrence rules from code or traces; rules are statistical and need many instances.
5. Crash as the oracle with directed delays:
   - Stoica, Lu, Musuvathi, Nath, "WAFFLE: Exposing Memory Ordering Bugs Efficiently with Active Delay
     Injection", EuroSys 2023: memory-order bugs (use before initialization, use after free); a preparation run
     finds candidate (init, use) pairs, then delays are injected to flip them; the oracle is the crash.
   - Li, Lu, et al., "Efficient Scalable Thread-Safety-Violation Detection" (TSVD), SOSP 2019: delay injection at
     near-miss call pairs on thread-unsafe objects.
   - Erickson, Musuvathi, Burckhardt, Olynyk, "DataCollider", OSDI 2010; Park, Lu, Zhou, "CTrigger", ASPLOS 2009;
     Sen, "RaceFuzzer", PLDI 2008; Burckhardt, Kothari, Musuvathi, Nagarakatte, "PCT", ASPLOS 2010.

## C. Framework- or event-level races (objects above raw memory)
- Raychev, Vechev, Sridharan, "EventRacer", OOPSLA 2013; Hsiao et al., "Race Detection for Event-Driven Mobile
  Applications" (CAFA), PLDI 2014; Maiya, Kanade, Majumdar, "DroidRacer", PLDI 2014: happens-before models built
  from framework semantics (callbacks, lifecycle), use-before-initialization and use-after-free across events;
  heavy false-positive filtering is the recurring issue.
- For component frameworks in robotics, automotive or flight software (ROS, AUTOSAR, F Prime, cFS) the lead knows
  of no detector for startup-order violations between components. [U: must be searched before any novelty claim]

## D. Observations for designers [I]
- Three oracle families exist without a specification: (i) a generic failure signal, (ii) a difference between
  the two orders, (iii) a learned or mined rule. (i) misses silent failures and flags designed error paths;
  (ii) needs a notion of equivalence that tolerates benign nondeterminism; (iii) needs many instances.
- Nearly all detectors separate "find candidate pairs" from "decide harm", and the strongest ones decide harm by
  running the other order.
- Flight-software frameworks label outcomes themselves (status codes, event severities, application exit
  status). None of the works above had such a per-call vocabulary available; it may be the lever.
