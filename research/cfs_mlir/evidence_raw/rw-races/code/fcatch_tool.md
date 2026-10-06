---
layout: default
---

# [](#header-1)Challenges
* Previous works require manual specifications.
* The state space is huge to explore.
* Traditional happens-before relationship is unassisted. :worried:

# [](#header-1)Key insights of FCatch

> TOF bugs = A problem of **Timing** + **Unexpected** states **shared** among nodes

# [](#header-1)FCatch detection flow
![](./fcatch-flow.svg)

{::comment}
graph LR
A[Observe correct runs] -|-> B(Logs)
B -|-> C[Identify conflicting ops]
C -|-> D(TOF bug candidates)
D -|-> E[Identify fault-intolerant ops]
E -|-> F(Reports)
{:/comment}

# [](#header-1)True TOF bugs found by FCatch
The details of all the true TOF bugs found by FCatch can be accessed in 3 ways:
* Read the document in [Github](https://github.com/haopeng-liu/TOF-bugs)

* Remotely access the VM and reproduce each bug
```
ssh -p2345 fcatch@toadette.cs.uchicago.edu
> password !QAZ2wsx
```
* Download the VM image
```
wget http://toadette.cs.uchicago.edu:5678/fcatch.vdi
> username: fcatch
> password: fcatch
```

