---
layout: default
---


# [](#header-1)What are Time-of-Fault (TOF) bugs?
* Three compositions:
  * Trigger: a special **timing** of fault [^1].
  * Error: **unexpected** state left due to the untimely fault.
  * Failure: system **cannot properly** handle such unexpected state.
* A real example: [MR-3858](https://issues.apache.org/jira/browse/MAPREDUCE-3858)
  * Trigger: bug happens only when a task attempt crashes in the middle of committing.
  * Error: the crashing attempt's ID was recorded in a heap object as the committing attempt.
  * Failure: all relaunched task attempts cannot commit and the job fails (hang).

---

# [](#header-1)What is FCatch?
FCatch is a research project aiming at fighting TOF bugs in cloud systems. Currently, it contains:
*     A [TOF bug model](model) and a [TOF bug benchmark suite](https://goo.gl/forms/qbGVz4bt8DFpU6F33).
*     A [tool](tool) to **automatically** predict TOF bugs from **correct runs** with low false positive rate. 

We are working on providing better solutions to make software systems more reliable, especially for distributed and cloud systems.

---

# [](#header-1)Publications
*     [FCatch: Automatically detecting time-of-fault bugs in cloud systems.](http://people.cs.uchicago.edu/~shanlu/paper/asplos18_fcatch.pdf) Haopeng Liu, Xu Wang, Guangpu Li, Shan Lu, Feng Ye and Chen Tian, _ASPLOS'18_
```
@inproceedings{liu2018fcatch,
  title={FCatch: Automatically Detecting Time-of-fault Bugs in Cloud Systems},
  author={Liu, Haopeng and Wang, Xu and Li, Guangpu and Lu, Shan and Ye, Feng and Tian, Chen},
  booktitle={Proceedings of the 23rd International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS'18)},
}
```

---
[^1]: message loss or node crashes ([How common are they?](./chart.jpg)).

<!---
This is a normal paragraph following a header. GitHub is a code hosting platform for version control and collaboration. It lets you and others work together on projects from anywhere.

## [](#header-2)Header 2

> This is a blockquote following a header.
>
> When something is important enough, you do it even if the odds are not in your favor.

### [](#header-3)Header 3

```js
// Javascript code with syntax highlighting.
var fun = function lang(l) {
  dateformat.i18n = require('./lang/' + l)
  return true;
}
```

```ruby
# Ruby code with syntax highlighting
GitHubPages::Dependencies.gems.each do |gem, version|
  s.add_dependency(gem, "= #{version}")
end
```

#### [](#header-4)Header 4

*   This is an unordered list following a header.
*   This is an unordered list following a header.
*   This is an unordered list following a header.

##### [](#header-5)Header 5

1.  This is an ordered list following a header.
2.  This is an ordered list following a header.
3.  This is an ordered list following a header.

###### [](#header-6)Header 6

| head1        | head two          | three |
|:-------------|:------------------|:------|
| ok           | good swedish fish | nice  |
| out of stock | good and plenty   | nice  |
| ok           | good `oreos`      | hmm   |
| ok           | good `zoute` drop | yumm  |

### There's a horizontal rule below this.

* * *

### Here is an unordered list:

*   Item foo
*   Item bar
*   Item baz
*   Item zip

### And an ordered list:

1.  Item one
1.  Item two
1.  Item three
1.  Item four

### And a nested list:

- level 1 item
  - level 2 item
  - level 2 item
    - level 3 item
    - level 3 item
- level 1 item
  - level 2 item
  - level 2 item
  - level 2 item
- level 1 item
  - level 2 item
  - level 2 item
- level 1 item

### Small image

![](https://assets-cdn.github.com/images/icons/emoji/octocat.png)

### Large image

![](https://guides.github.com/activities/hello-world/branching.png)


### Definition lists can be used with HTML syntax.

<dl>
<dt>Name</dt>
<dd>Godzilla</dd>
<dt>Born</dt>
<dd>1952</dd>
<dt>Birthplace</dt>
<dd>Japan</dd>
<dt>Color</dt>
<dd>Green</dd>
</dl>

```
Long, single-line code blocks should not wrap. They should horizontally scroll if they are too long. This line should be long enough to demonstrate this.
```

```
The final element.
```
--->
