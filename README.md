# swege
swege is a Static WEbsite GEnerator written in C.

It leverages the [discount](http://www.pell.portland.or.us/~orc/Code/discount/)
library for generating a website from a set of Markdown files.

# FEATURES
* Under 300 lines of C.
* Incremental updates.
* Pretty fast!
* Almost no dependencies except for discount.
* Portable-ish (*nix at least).

# QUICK START
Edit config.h with appropriate values for your website. Then, simply run
swege outside of your website's source directory.

# LICENSE
Copyright 2019 Sergei Akhmatdinov

Licensed under the Apache License, Version 2.0 (the "License");
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

# CREDITS
Many thanks to David Parsons for the Discount library.

# BUGS/TODO
* Add support for ini files. (ini.h?)
* Add a way to generate a skeleton website with a command (swege new?)
* Better documentation.
* Windows support?

# CONTRIBUTING
Contributions are welcome no matter who you are and where you come from.

When submitting PRs, please maintain the [coding style](https://suckless.org/coding_style/)
used for the project.
