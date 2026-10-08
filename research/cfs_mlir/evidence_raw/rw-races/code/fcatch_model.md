---
layout: default
---

# [](#header-1) Fundamental principle
TOF, a nondeterminism culprit, introduces new define-use relations between _W_ and _R_ beyond traditional concurrency.

# [](#header-1)TOF bug categorization

|:---:|:---:|:---:|
| _Conflicting_ | _R_<sub>Regular</sub> | _R_<sub>Recovery</sub> |
| _W_<sub>Crash</sub> | **Crash-regular TOF bug** | **Crash-recovery TOF bug** |


* _W_<sub>Crash</sub>: _W_ from[^1] the crash node
* _R_<sub>Regular</sub>: _R_ from[^1] a non-crash node
* _R_<sub>Recovery</sub>: _R_ from[^1] the recovery node

---

# [](#header-1)Crash-regular TOF bugs
*  How can TOF determine the content consumed by _R_<sub>Regular</sub>?
    * _R_<sub>Regular</sub> happens-before _W_<sub>Crash</sub>? :worried:
    * _W_<sub>Crash</sub> happens-before _R_<sub>Regular</sub>? :yum:
    * _W_<sub>Crash</sub> concurrent with _R_<sub>Regular</sub>? :worried:
![](./crash-regular.png)
* Fault-tolerant mechanism: timeout

---

# [](#header-1)Crash-recovery TOF bugs
*  The content consumed by _R_<sub>Recovery</sub> is **completely** determined by TOF.
![](./crash-recovery.png)
* Fault-tolerance mechanisms: sanity check and data overwrite

---
[^1]: "from" means _W_/_R_ physically executes on the node **or** casually initiated by the node.
