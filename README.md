Sdust is a reimplementation of the [symmetric DUST algorithm][paper] for
finding low-complexity regions in DNA sequences. It gives identical output to
[NCBI's dustmasker][dm] except in assembly gaps, and is four times as fast. The
source code here was initially written for [minimap][mm] and later
[minimap2][mm2]. This repo is a standalone copy.

[paper]: http://www.ncbi.nlm.nih.gov/pubmed/16796549
[dm]: http://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/app/dustmasker/
[mm]: https://github.com/lh3/minimap
[mm2]: https://github.com/lh3/minimap2

* Parameter

Parameter | Type | Description | Default
-----|------|--------------------|---
  `-w` | INT  | Dust window length | 64
  `-t` | INT  | Dust level (score threshold for subwindows) | 20
  `-l` | INT  | Minimum segment length to dust | 7
  `-m` | CHAR | Mask dusted sequences (works with -d) with character X or N | uncapitalized
  `-d` |      | Output sequences instead of dust bed intervals | bed interval
  `-c` |      | Wrapping sequence at specified column | no wrapping
  `-n` | INT| Maximum mask length to output (works with -d) | no limit |
  `-h` |      | Display this help | &nbsp;
  `-v` |      |  Show program version | &nbsp;
